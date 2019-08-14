#include "pch.h"

#include "App.h"
#include "MainPage.h"

using namespace winrt;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace HLSLComputePlayground;
using namespace HLSLComputePlayground::implementation;

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    InitializeComponent();
    Suspending({ this, &App::OnSuspending });

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([this](IInspectable const&, UnhandledExceptionEventArgs const& e)
    {
        if (IsDebuggerPresent())
        {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

static winrt::Windows::Foundation::IAsyncAction run() {
	com_ptr<ID3D12Debug> debug;
	com_ptr<ID3D12Debug1> debug1;
	check_hresult(D3D12GetDebugInterface(__uuidof(debug), debug.put_void()));
	check_hresult(debug->QueryInterface(__uuidof(debug1), debug1.put_void()));
	debug1->EnableDebugLayer();
	debug1->SetEnableGPUBasedValidation(true);

	com_ptr<IDXGIFactory4> dxgiFactory;
	check_hresult(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, __uuidof(dxgiFactory), dxgiFactory.put_void()));

	com_ptr<IDXGIAdapter1> dxgiAdapter;
	check_hresult(dxgiFactory->EnumAdapters1(0, dxgiAdapter.put()));

	com_ptr<ID3D12Device> device;
	check_hresult(D3D12CreateDevice(dxgiAdapter.get(), D3D_FEATURE_LEVEL_12_1, __uuidof(device), device.put_void()));

	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsUnorderedAccessView(0);
	rootParameters[1].InitAsUnorderedAccessView(1);
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters);
	com_ptr<ID3DBlob> signature;
	com_ptr<ID3DBlob> error;
	check_hresult(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1_1, signature.put(), error.put()));

	com_ptr<ID3D12RootSignature> rootSignature;
	check_hresult(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(rootSignature), rootSignature.put_void()));

	auto folder = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation();
	auto file = co_await folder.GetFileAsync(L"ComputeShader.cso");
	auto fileBuffer = co_await winrt::Windows::Storage::FileIO::ReadBufferAsync(file);
	std::vector<byte> bytecode;
	bytecode.resize(fileBuffer.Length());
	winrt::Windows::Storage::Streams::DataReader::FromBuffer(fileBuffer).ReadBytes(bytecode);

	D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDescriptor = {};
	pipelineStateDescriptor.pRootSignature = rootSignature.get();
	pipelineStateDescriptor.CS = CD3DX12_SHADER_BYTECODE(bytecode.data(), bytecode.size());
	com_ptr<ID3D12PipelineState> pipelineState;
	check_hresult(device->CreateComputePipelineState(&pipelineStateDescriptor, __uuidof(pipelineState), pipelineState.put_void()));

	float rawBuffer1Data[] = { -0.0, 0.0 };
	constexpr int numFloatsUpload = _countof(rawBuffer1Data);
	constexpr int numFloatsReadback = 1;

	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto readbackHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	CD3DX12_RESOURCE_DESC matrixResourceDescription = CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * numFloatsUpload);
	CD3DX12_RESOURCE_DESC matrixResourceDescriptionUAV = CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * numFloatsUpload, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	CD3DX12_RESOURCE_DESC floatResourceDescription = CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * numFloatsReadback);
	CD3DX12_RESOURCE_DESC floatResourceDescriptionUAV = CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * numFloatsReadback, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	com_ptr<ID3D12Resource> buffer1;
	check_hresult(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &matrixResourceDescriptionUAV, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(buffer1), buffer1.put_void()));
	com_ptr<ID3D12Resource> buffer1Upload;
	check_hresult(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &matrixResourceDescription, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(buffer1Upload), buffer1Upload.put_void()));

	com_ptr<ID3D12Resource> buffer2;
	check_hresult(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &floatResourceDescriptionUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, __uuidof(buffer2), buffer2.put_void()));
	com_ptr<ID3D12Resource> buffer2Download;
	check_hresult(device->CreateCommittedResource(&readbackHeapProperties, D3D12_HEAP_FLAG_NONE, &floatResourceDescription, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(buffer2Download), buffer2Download.put_void()));

	com_ptr<ID3D12CommandQueue> commandQueue;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	check_hresult(device->CreateCommandQueue(&queueDesc, __uuidof(commandQueue), commandQueue.put_void()));

	com_ptr<ID3D12CommandAllocator> commandAllocator;
	check_hresult(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(commandAllocator), commandAllocator.put_void()));

	com_ptr<ID3D12GraphicsCommandList> commandList;
	check_hresult(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.get(), nullptr, __uuidof(commandList), commandList.put_void()));

	com_ptr<ID3D12Fence> fence;
	check_hresult(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(fence), fence.put_void()));
	auto fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	D3D12_SUBRESOURCE_DATA buffer1Data = {};
	buffer1Data.pData = &rawBuffer1Data;
	buffer1Data.RowPitch = sizeof(float) * numFloatsUpload;
	buffer1Data.SlicePitch = buffer1Data.RowPitch;
	UpdateSubresources(commandList.get(), buffer1.get(), buffer1Upload.get(), 0, 0, 1, &buffer1Data);
	CD3DX12_RESOURCE_BARRIER barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(buffer1.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->ResourceBarrier(1, &barrier1);

	commandList->SetPipelineState(pipelineState.get());
	commandList->SetComputeRootSignature(rootSignature.get());
	commandList->SetComputeRootUnorderedAccessView(0, buffer1->GetGPUVirtualAddress());
	commandList->SetComputeRootUnorderedAccessView(1, buffer2->GetGPUVirtualAddress());
	commandList->Dispatch(1, 1, 1);

	CD3DX12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(buffer2.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->ResourceBarrier(1, &barrier2);
	commandList->CopyResource(buffer2Download.get(), buffer2.get());

	check_hresult(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList.get() };
	commandQueue->ExecuteCommandLists(1, commandLists);

	check_hresult(commandQueue->Signal(fence.get(), 1));
	check_hresult(fence->SetEventOnCompletion(1, fenceEvent));
	WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

	D3D12_RANGE readbackBufferRange{ 0, sizeof(float) * numFloatsReadback };
	void* readbackBufferData = nullptr;
	check_hresult(buffer2Download->Map(0, &readbackBufferRange, &readbackBufferData));
	float result[numFloatsReadback];
	memcpy(result, readbackBufferData, sizeof(float) * numFloatsReadback);
	D3D12_RANGE emptyRange{ 0, 0 };
	buffer2Download->Unmap(0, &emptyRange);

	{
		std::wstringstream ss;
		for (int i = 0; i < numFloatsReadback; ++i)
			ss << i << ": " << result[i] << std::endl;
		OutputDebugString(ss.str().c_str());
	}
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs const& e)
{
	run();

    Frame rootFrame{ nullptr };
    auto content = Window::Current().Content();
    if (content)
    {
        rootFrame = content.try_as<Frame>();
    }

    // Do not repeat app initialization when the Window already has content,
    // just ensure that the window is active
    if (rootFrame == nullptr)
    {
        // Create a Frame to act as the navigation context and associate it with
        // a SuspensionManager key
        rootFrame = Frame();

        rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });

        if (e.PreviousExecutionState() == ApplicationExecutionState::Terminated)
        {
            // Restore the saved session state only when appropriate, scheduling the
            // final launch steps after the restore is complete
        }

        if (e.PrelaunchActivated() == false)
        {
            if (rootFrame.Content() == nullptr)
            {
                // When the navigation stack isn't restored navigate to the first page,
                // configuring the new page by passing required information as a navigation
                // parameter
                rootFrame.Navigate(xaml_typename<HLSLComputePlayground::MainPage>(), box_value(e.Arguments()));
            }
            // Place the frame in the current Window
            Window::Current().Content(rootFrame);
            // Ensure the current window is active
            Window::Current().Activate();
        }
    }
    else
    {
        if (e.PrelaunchActivated() == false)
        {
            if (rootFrame.Content() == nullptr)
            {
                // When the navigation stack isn't restored navigate to the first page,
                // configuring the new page by passing required information as a navigation
                // parameter
                rootFrame.Navigate(xaml_typename<HLSLComputePlayground::MainPage>(), box_value(e.Arguments()));
            }
            // Ensure the current window is active
            Window::Current().Activate();
        }
    }
}

/// <summary>
/// Invoked when application execution is being suspended.  Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
/// <param name="sender">The source of the suspend request.</param>
/// <param name="e">Details about the suspend request.</param>
void App::OnSuspending([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] SuspendingEventArgs const& e)
{
    // Save application state and stop any background activity
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(IInspectable const&, NavigationFailedEventArgs const& e)
{
    throw hresult_error(E_FAIL, hstring(L"Failed to load Page ") + e.SourcePageType().Name);
}