#include <stdio.h>
#include <stdlib.h>

#include <CL/cl.h>

#define MAX_PLATFORM_IDS 1024
#define MAX_BUFFER_SIZE  1024

int main(int argc, char* argv[])
{
	cl_platform_id platform_ids[MAX_PLATFORM_IDS];
	cl_uint num_platform_ids;
	cl_device_id device_ids[MAX_PLATFORM_IDS];
	cl_uint num_device_ids;
	cl_int ret;
	char buffer[MAX_BUFFER_SIZE];
	cl_uint buf_uint;
	cl_uint buf_ulong;
	cl_device_type buf_type;

	ret = clGetPlatformIDs(MAX_PLATFORM_IDS, platform_ids, &num_platform_ids);

	fprintf(stderr, "num of platform ids: %d\n", num_platform_ids);

	for(int i = 0; i < num_platform_ids; i++)
	{
		fprintf(stderr, "-<%d>------------------- Platform ID[%p]\n", i, platform_ids[i]);

		ret = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_PROFILE, MAX_BUFFER_SIZE, buffer, NULL);
		fprintf(stderr, "CL_PLATFORM_PROFILE = %s\n", buffer);

		ret = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VERSION, MAX_BUFFER_SIZE, buffer, NULL);
		fprintf(stderr, "CL_PLATFORM_VERSION = %s\n", buffer);

		ret = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_NAME, MAX_BUFFER_SIZE, buffer, NULL);
		fprintf(stderr, "CL_PLATFORM_NAME = %s\n", buffer);

		ret = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR, MAX_BUFFER_SIZE, buffer, NULL);
		fprintf(stderr, "CL_PLATFORM_VENDOR = %s\n", buffer);

		ret = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_EXTENSIONS, MAX_BUFFER_SIZE, buffer, NULL);
		fprintf(stderr, "CL_PLATFORM_EXTENSIONS = %s\n", buffer);
#if 1
		ret = clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL, MAX_PLATFORM_IDS, device_ids, &num_device_ids);

		fprintf(stderr, "\t");
		fprintf(stderr, "num of device ids: %d\n", num_device_ids);

		for(int i = 0; i < num_device_ids; i++)
		{
			fprintf(stderr, "\t");
			fprintf(stderr, "-<%d>------------------- Device ID[%p]\n", i, device_ids[i]);

			ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_NAME, MAX_BUFFER_SIZE, buffer, NULL);
			fprintf(stderr, "\t");
			fprintf(stderr, "CL_DEVICE_NAME = %s\n", buffer);

			ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_TYPE, sizeof(buf_type), &buf_type, NULL);
			fprintf(stderr, "\t");
			fprintf(stderr, "CL_DEVICE_TYPE = %x\n", buf_type);

			ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_VENDOR, MAX_BUFFER_SIZE, buffer, NULL);
			fprintf(stderr, "\t");
			fprintf(stderr, "CL_DEVICE_VENDOR = %s\n", buffer);

			ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_VERSION, MAX_BUFFER_SIZE, buffer, NULL);
			fprintf(stderr, "\t");
			fprintf(stderr, "CL_DEVICE_VERSION = %s\n", buffer);

			ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL);
			fprintf(stderr, "\t");
			fprintf(stderr, "CL_DEVICE_MAX_COMPUTE_UNITS = %d\n", buf_uint);

			ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL);
			fprintf(stderr, "\t");
			fprintf(stderr, "CL_DEVICE_GLOBAL_MEM_SIZE = %d\n", buf_ulong);
		}
#endif
	}

	return 0;
}
