#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include <CL/cl.h>

//#include <pgm/pgm.h>
#include "pgm.h" 

#define MAX_PLATFORM_IDS 1024
#define MAX_DEVICE_IDS 1024

#define PI 3.14159265358979

#define MAX_SOURCE_SIZE (0x1000000)

#define AMP(a, b)  (sqrt((a)*(a) + (b)*(b)))

#define CL_CHECK(X) 					\
	if(X != CL_SUCCESS)				\
	{						\
		fprintf(stderr, "\33[31m [%s:%d:%s()] ERROR(%d)!\33[m\n", __FILE__, __LINE__, __func__, X); 	\
		exit(1);				\
	}						\

#define LOG(fmt, args...)  \
		fprintf(stderr, "[%s:%d:%s()] " fmt, __FILE__, __LINE__, __func__, ##args); 	\

cl_platform_id platform_ids[MAX_PLATFORM_IDS];
cl_platform_id platform_id;
cl_uint num_platform_ids;

cl_device_id device_ids[MAX_PLATFORM_IDS];
cl_uint num_device_ids;

cl_device_id device_id = NULL;
cl_context context = NULL;
cl_command_queue queue = NULL;
cl_program program = NULL;


enum Mode {
	forward = 0,
	inverse = 1,
};

int setWorkSize(size_t *gws, size_t *lws, cl_int x, cl_int y)
{
	switch (y)
	{
		case 1:
			gws[0] = x;
			gws[1] = 1;
			lws[0] = 1;
			lws[1] = 1;
			break;
		default:
			gws[0] = x;
			gws[1] = y;
			lws[0] = 1;
			lws[1] = 1;
			break;
	}
}

int fftCore(cl_mem dst, cl_mem src, cl_mem spin, cl_int m, enum Mode direction)
{
	cl_int ret;

	cl_int iter;
	cl_uint flag;

	cl_int n = 1 << m;

	cl_event kernelDone;

	cl_kernel brev = NULL;
	cl_kernel bfly = NULL;
	cl_kernel norm = NULL;

	brev = clCreateKernel(program, "bitReverse", &ret);
	CL_CHECK(ret);
	bfly = clCreateKernel(program, "butterfly", &ret);
	CL_CHECK(ret);
	norm = clCreateKernel(program, "norm", &ret);
	CL_CHECK(ret);

	size_t gws[2];
	size_t lws[2];

	switch (direction)
	{
		case forward: flag = 0x00000000; break;
		case inverse: flag = 0x80000000; break;
	}

	CL_CHECK(ret = clSetKernelArg(brev, 0, sizeof(cl_mem), (void *)&dst));
	CL_CHECK(ret = clSetKernelArg(brev, 1, sizeof(cl_mem), (void *)&src));
	CL_CHECK(ret = clSetKernelArg(brev, 2, sizeof(cl_int), (void *)&m));
	CL_CHECK(ret = clSetKernelArg(brev, 3, sizeof(cl_int), (void *)&n));

	CL_CHECK(ret = clSetKernelArg(bfly, 0, sizeof(cl_mem), (void *)&dst));
	CL_CHECK(ret = clSetKernelArg(bfly, 1, sizeof(cl_mem), (void *)&spin));
	CL_CHECK(ret = clSetKernelArg(bfly, 2, sizeof(cl_int), (void *)&m));
	CL_CHECK(ret = clSetKernelArg(bfly, 3, sizeof(cl_int), (void *)&n));
	CL_CHECK(ret = clSetKernelArg(bfly, 5, sizeof(cl_uint), (void *)&flag));

	CL_CHECK(ret = clSetKernelArg(norm, 0, sizeof(cl_mem), (void *)&dst));
	CL_CHECK(ret = clSetKernelArg(norm, 1, sizeof(cl_int), (void *)&n));

	setWorkSize(gws, lws, n, n);
	CL_CHECK(ret = clEnqueueNDRangeKernel(queue, brev, 2, NULL, gws, lws, 0, NULL, NULL));

	setWorkSize(gws, lws, n/2, n);
	for(iter = 1; iter <= m; iter++)
	{
		CL_CHECK(ret = clSetKernelArg(bfly, 4, sizeof(cl_int), (void *)&iter));
		CL_CHECK(ret = clEnqueueNDRangeKernel(queue, bfly, 2, NULL, gws, lws, 0, NULL, &kernelDone));
		CL_CHECK(ret = clWaitForEvents(1, &kernelDone));
	}

	if(direction == inverse)
	{
		setWorkSize(gws, lws, n, n);
		CL_CHECK(ret = clEnqueueNDRangeKernel(queue, norm, 2, NULL, gws, lws, 0, NULL, &kernelDone));
		CL_CHECK(ret = clWaitForEvents(1, &kernelDone));
	}

	CL_CHECK(ret = clReleaseKernel(bfly));
	CL_CHECK(ret = clReleaseKernel(brev));
	CL_CHECK(ret = clReleaseKernel(norm));

	return 0;
}

int main(int argc, char *argv[])
{
	//fprintf(stderr, "[%s:%d:%s()] FFT!\n", __FILE__, __LINE__, __func__);
	LOG("FFT Start\n");
	cl_mem xmobj = NULL;
	cl_mem rmobj = NULL;
	cl_mem wmobj = NULL;
	cl_kernel sfac = NULL;
	cl_kernel trns = NULL;
	cl_kernel hpfl = NULL;

	cl_uint ret_num_platforms;
	cl_uint ret_num_devices;

	cl_int ret;

	cl_float2 *xm;
	cl_float2 *rm;
	cl_float2 *wm;

	pgm_t ipgm;
	pgm_t opgm;

	FILE *fp;
	const char fileName[] = "./fft.cl";
	size_t source_size;
	char *source_str;
	cl_int i, j;
	cl_int n;
	cl_int m;

	size_t gws[2];
	size_t lws[2];

	fp = fopen(fileName, "r");
	if(!fp)
	{
		fprintf(stderr, "[%s:%d:%s()] ERROR, Failed to load kernel source.\n", __FILE__, __LINE__, __func__);
		return 1;
	}

	source_str = (char *)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	readPGM(&ipgm, "./lena.pgm");

	n = ipgm.width;
	m = (cl_int)(log((double)n)/log(2.0));

	LOG("n = %d, m = %d.\n", m, n);

	xm = (cl_float2*)malloc(n*n*sizeof(cl_float2));
	rm = (cl_float2*)malloc(n*n*sizeof(cl_float2));
	wm = (cl_float2*)malloc(n/2 *sizeof(cl_float2));

	for( i = 0; i < n; i++)
	{
		for(j = 0; j < n; j++)
		{
			((float*)xm)[2*(n*j + i) + 0] = (float)ipgm.buf[n*j + i];
			((float*)xm)[2*(n*j + i) + 1] = (float)0;
		}
	}

	CL_CHECK(ret = clGetPlatformIDs(MAX_PLATFORM_IDS, platform_ids, &ret_num_platforms));
	platform_id = platform_ids[0];
	CL_CHECK(ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices));

	LOG("platform_id = %p, device_id = %p\n", platform_id, device_id);


	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
	CL_CHECK(ret);

	queue = clCreateCommandQueue(context, device_id, 0, &ret);

	xmobj = clCreateBuffer(context, CL_MEM_READ_WRITE, n*n*sizeof(cl_float2), NULL, &ret);
	CL_CHECK(ret);
	rmobj = clCreateBuffer(context, CL_MEM_READ_WRITE, n*n*sizeof(cl_float2), NULL, &ret);
	CL_CHECK(ret);
	wmobj = clCreateBuffer(context, CL_MEM_READ_WRITE, n*n*sizeof(cl_float2), NULL, &ret);
	CL_CHECK(ret);

	CL_CHECK(ret = clEnqueueWriteBuffer(queue, xmobj, CL_TRUE, 0, n*n*sizeof(cl_float2), xm, 0, NULL, NULL));

	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);
	CL_CHECK(ret);

	CL_CHECK(ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL));

	sfac = clCreateKernel(program, "spinFact", &ret);
	CL_CHECK(ret);
	trns = clCreateKernel(program, "transpose", &ret);
	CL_CHECK(ret);
	hpfl = clCreateKernel(program, "highPassFilter", &ret);
	CL_CHECK(ret);

	CL_CHECK(ret = clSetKernelArg(sfac, 0, sizeof(cl_mem), (void *)&wmobj));
	CL_CHECK(ret = clSetKernelArg(sfac, 1, sizeof(cl_int), (void *)&n));
	setWorkSize(gws, lws, n/2, 1);
	CL_CHECK(ret = clEnqueueNDRangeKernel(queue, sfac, 1, NULL, gws, lws, 0, NULL, NULL));

	fftCore(rmobj, xmobj, wmobj, m, forward);

	CL_CHECK(ret = clSetKernelArg(trns, 0, sizeof(cl_mem), (void *)&xmobj));
	CL_CHECK(ret = clSetKernelArg(trns, 1, sizeof(cl_mem), (void *)&rmobj));
	CL_CHECK(ret = clSetKernelArg(trns, 2, sizeof(cl_int), (void *)&n));
	setWorkSize(gws, lws, n, n);
	CL_CHECK(ret = clEnqueueNDRangeKernel(queue, trns, 2, NULL, gws, lws, 0, NULL, NULL));

	fftCore(rmobj, xmobj, wmobj, m, forward);

#if 1 //FILTER
	cl_int radius = n>>4;
	CL_CHECK(ret = clSetKernelArg(hpfl, 0, sizeof(cl_mem), (void *)&rmobj));
	CL_CHECK(ret = clSetKernelArg(hpfl, 1, sizeof(cl_int), (void *)&n));
	CL_CHECK(ret = clSetKernelArg(hpfl, 2, sizeof(cl_int), (void *)&radius));
	setWorkSize(gws, lws, n, n);
	CL_CHECK(ret = clEnqueueNDRangeKernel(queue, hpfl, 2, NULL, gws, lws, 0, NULL, NULL));
#endif

#if 1 /* Inverse FFT */
	fftCore(xmobj, rmobj, wmobj, m, inverse);

	CL_CHECK(ret = clSetKernelArg(trns, 0, sizeof(cl_mem), (void *)&rmobj));
	CL_CHECK(ret = clSetKernelArg(trns, 1, sizeof(cl_mem), (void *)&xmobj));
	CL_CHECK(ret = clSetKernelArg(trns, 2, sizeof(cl_int), (void *)&n));
	setWorkSize(gws, lws, n, n);
	CL_CHECK(ret = clEnqueueNDRangeKernel(queue, trns, 2, NULL, gws, lws, 0, NULL, NULL));

	fftCore(xmobj, rmobj, wmobj, m, inverse);
#endif

	CL_CHECK(ret = clEnqueueReadBuffer(queue, xmobj, CL_TRUE, 0, n*n*sizeof(cl_float2), xm, 0, NULL, NULL));

	float *ampd;
	ampd = (float*)malloc(n*n*sizeof(float));
	for(i = 0; i < n; i++)
	{
		for(j = 0; j < n; j++)
		{
			ampd[n*i + j] = AMP( ((float*)xm)[2*(n*i + j)], ((float*)xm)[2*(n*i + j) + 1] );
//			fprintf(stderr, "%d ", (int)ampd[n*i + j]);
		}
//		fprintf(stderr, "\n");
	}

	opgm.width = n;
	opgm.height = n;
	normalizeF2PGM(&opgm, ampd);
	free(ampd);

	writePGM(&opgm, "output.pgm");

	/* Termination */
	CL_CHECK(ret = clFlush(queue));
	CL_CHECK(ret = clFinish(queue));
	CL_CHECK(ret = clReleaseKernel(hpfl));
	CL_CHECK(ret = clReleaseKernel(trns));
	CL_CHECK(ret = clReleaseKernel(sfac));
	CL_CHECK(ret = clReleaseProgram(program));
	CL_CHECK(ret = clReleaseMemObject(xmobj));
	CL_CHECK(ret = clReleaseMemObject(rmobj));
	CL_CHECK(ret = clReleaseMemObject(wmobj));
	CL_CHECK(ret = clReleaseCommandQueue(queue));
	CL_CHECK(ret = clReleaseContext(context));

	destroyPGM(&ipgm);
	destroyPGM(&opgm);

	free(source_str);
	free(wm);
	free(rm);
	free(xm);

	return 0;
}


