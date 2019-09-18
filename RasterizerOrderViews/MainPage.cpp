#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace winrt::Windows::UI::Xaml;

namespace winrt::RasterizerOrderViews::implementation
{
	static winrt::Windows::Foundation::IAsyncAction run() {
		UINT width = 16;
		UINT height = 16;

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
		check_hresult(D3D12CreateDevice(dxgiAdapter.get(), D3D_FEATURE_LEVEL_12_0, __uuidof(device), device.put_void()));

		CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		rootParameters[0].InitAsDescriptorTable(1, ranges);
		rootParameters[1].InitAsConstants(2, 0);
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // Only the input assembler stage needs access to the constant buffer.
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
		com_ptr<ID3DBlob> signature;
		com_ptr<ID3DBlob> error;
		check_hresult(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1_1, signature.put(), error.put()));

		com_ptr<ID3D12RootSignature> rootSignature;
		check_hresult(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(rootSignature), rootSignature.put_void()));

		std::vector<byte> vertexShaderBytecode;
		{
			auto folder = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation();
			auto file = co_await folder.GetFileAsync(L"VertexShader.cso");
			auto fileBuffer = co_await winrt::Windows::Storage::FileIO::ReadBufferAsync(file);
			vertexShaderBytecode.resize(fileBuffer.Length());
			winrt::Windows::Storage::Streams::DataReader::FromBuffer(fileBuffer).ReadBytes(vertexShaderBytecode);
		}
		std::vector<byte> pixelShaderBytecode;
		{
			auto folder = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation();
			auto file = co_await folder.GetFileAsync(L"PixelShader.cso");
			auto fileBuffer = co_await winrt::Windows::Storage::FileIO::ReadBufferAsync(file);
			pixelShaderBytecode.resize(fileBuffer.Length());
			winrt::Windows::Storage::Streams::DataReader::FromBuffer(fileBuffer).ReadBytes(pixelShaderBytecode);
		}

		static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDescriptor = {};
		pipelineStateDescriptor.pRootSignature = rootSignature.get();
		pipelineStateDescriptor.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBytecode.data(), vertexShaderBytecode.size());
		pipelineStateDescriptor.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBytecode.data(), pixelShaderBytecode.size());
		pipelineStateDescriptor.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pipelineStateDescriptor.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineStateDescriptor.DepthStencilState.DepthEnable = false;
		pipelineStateDescriptor.DepthStencilState.StencilEnable = false;
		pipelineStateDescriptor.InputLayout = { inputLayout, _countof(inputLayout) };
		pipelineStateDescriptor.SampleMask = UINT_MAX;
		pipelineStateDescriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateDescriptor.NumRenderTargets = 0;
		pipelineStateDescriptor.DSVFormat = DXGI_FORMAT_UNKNOWN;
		pipelineStateDescriptor.SampleDesc.Count = 1;

		com_ptr<ID3D12PipelineState> pipelineState;
		check_hresult(device->CreateGraphicsPipelineState(&pipelineStateDescriptor, __uuidof(pipelineState), pipelineState.put_void()));

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

		com_ptr<ID3D12DescriptorHeap> descriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescriptor = {};
		descriptorHeapDescriptor.NumDescriptors = 1;
		descriptorHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		check_hresult(device->CreateDescriptorHeap(&descriptorHeapDescriptor, __uuidof(descriptorHeap), descriptorHeap.put_void()));

		auto overlappingTriangleCount = 100;

		com_ptr<ID3D12Resource> vertexBuffer;
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(DirectX::XMFLOAT2) * 6 * overlappingTriangleCount);
		check_hresult(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(vertexBuffer), vertexBuffer.put_void()));
		com_ptr<ID3D12Resource> vertexBufferUpload;
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		check_hresult(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(vertexBufferUpload), vertexBufferUpload.put_void()));

		std::vector<DirectX::XMFLOAT2> vertexData(6 * overlappingTriangleCount);
		for (int i = 0; i < overlappingTriangleCount; ++i) {
			vertexData[6 * i + 0] = DirectX::XMFLOAT2(-1, -1);
			vertexData[6 * i + 1] = DirectX::XMFLOAT2(-1, 1);
			vertexData[6 * i + 2] = DirectX::XMFLOAT2(1, -1);
			vertexData[6 * i + 3] = DirectX::XMFLOAT2(1, -1);
			vertexData[6 * i + 4] = DirectX::XMFLOAT2(-1, 1);
			vertexData[6 * i + 5] = DirectX::XMFLOAT2(1, 1);
		}

		{
			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = vertexData.data();
			subresourceData.RowPitch = sizeof(DirectX::XMFLOAT2) * 6 * overlappingTriangleCount;
			subresourceData.SlicePitch = subresourceData.RowPitch;
			UpdateSubresources(commandList.get(), vertexBuffer.get(), vertexBufferUpload.get(), 0, 0, 1, &subresourceData);
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			commandList->ResourceBarrier(1, &barrier);
		}

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(DirectX::XMFLOAT2);
		vertexBufferView.SizeInBytes = sizeof(DirectX::XMFLOAT2) * 6 * overlappingTriangleCount;

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8_UINT;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		com_ptr<ID3D12Resource> texture;
		check_hresult(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(texture), texture.put_void()));
		com_ptr<ID3D12Resource> textureUpload;
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.get(), 0, 1);
		check_hresult(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(textureUpload), textureUpload.put_void()));
		CD3DX12_HEAP_PROPERTIES readbackHeapProperties(D3D12_HEAP_TYPE_READBACK);
		com_ptr<ID3D12Resource> textureDownload;
		check_hresult(device->CreateCommittedResource(&readbackHeapProperties, D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(textureDownload), textureDownload.put_void()));
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		UINT numRows;
		UINT64 rowSizeInBytes;
		UINT64 totalBytes;
		device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescriptor = {};
		uavDescriptor.Format = DXGI_FORMAT_R8_UINT;
		uavDescriptor.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDescriptor.Texture2D.MipSlice = 0;
		uavDescriptor.Texture2D.PlaneSlice = 0;
		device->CreateUnorderedAccessView(texture.get(), nullptr, &uavDescriptor, descriptorHeap->GetCPUDescriptorHandleForHeapStart());

		std::vector<UINT8> textureData(width * height * sizeof(UINT8));
		for (auto& t : textureData)
			t = std::numeric_limits<UINT8>::max() * 0.25;

		{
			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = textureData.data();
			subresourceData.RowPitch = width * sizeof(UINT8);
			subresourceData.SlicePitch = subresourceData.RowPitch * height;
			UpdateSubresources(commandList.get(), texture.get(), textureUpload.get(), 0, 0, 1, &subresourceData);
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			commandList->ResourceBarrier(1, &barrier);
		}

		commandList->SetPipelineState(pipelineState.get());
		commandList->SetGraphicsRootSignature(rootSignature.get());
		ID3D12DescriptorHeap* heaps[] = { descriptorHeap.get() };
		commandList->SetDescriptorHeaps(_countof(heaps), heaps);
		commandList->SetGraphicsRootDescriptorTable(0, descriptorHeap->GetGPUDescriptorHandleForHeapStart());
		UINT uniformBufferData[] = { width, height };
		commandList->SetGraphicsRoot32BitConstants(1, 2, uniformBufferData, 0);
		D3D12_VIEWPORT viewport = { 0, 0, width, height, 0, 1 };
		commandList->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->DrawInstanced(6 * overlappingTriangleCount, 1, 0, 0);

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->ResourceBarrier(1, &barrier);
		CD3DX12_TEXTURE_COPY_LOCATION Dst(textureDownload.get(), footprint);
		CD3DX12_TEXTURE_COPY_LOCATION Src(texture.get(), 0); 
		commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

		check_hresult(commandList->Close());
		ID3D12CommandList* commandLists[] = { commandList.get() };
		commandQueue->ExecuteCommandLists(1, commandLists);
		check_hresult(commandQueue->Signal(fence.get(), 1));
		check_hresult(fence->SetEventOnCompletion(1, fenceEvent));
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

		D3D12_RANGE readbackBufferRange{ 0, uploadBufferSize };
		void* readbackBufferData = nullptr;
		check_hresult(textureDownload->Map(0, &readbackBufferRange, &readbackBufferData));
		std::vector<UINT8> result(width * height);
		for (UINT i = 0; i < height; ++i)
			memcpy(result.data() + i * width, reinterpret_cast<char*>(readbackBufferData) + footprint.Footprint.RowPitch * i, width * sizeof(UINT8));
		D3D12_RANGE emptyRange{ 0, 0 };
		textureDownload->Unmap(0, &emptyRange);

		std::ostringstream ss;
		for (UINT8 i : result)
			ss << static_cast<int>(i) << ", ";
		ss << std::endl;
		OutputDebugStringA(ss.str().c_str());
	}

    MainPage::MainPage()
    {
        InitializeComponent();

		auto t = run();
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        myButton().Content(box_value(L"Clicked"));
    }
}
