#version 450

layout(std430, binding = 0) buffer Buffer1 {
	mat4 inputData[];
};

layout(std430, binding = 1) buffer Buffer2 {
	mat4 outputData[];
};

layout(local_size_x = 1) in;

void main() {
	outputData[0] = inputData[0] * inputData[1];
}
