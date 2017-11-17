/*

 Copyright (c) 2014 University of Edinburgh, Imperial College, University of Manchester.
 Developed in the PAMELA project, EPSRC Programme Grant EP/K008730/1

 This code is licensed under the MIT License.

 */
#include <kernels.h>

#include <drake.h>
#include <drake/intel-ia.h>
#include <drake-kfusion.h>

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>

	
	#define TICK()    {if (print_kernel_timing) {\
		host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);\
		clock_get_time(cclock, &tick_clockData);\
		mach_port_deallocate(mach_task_self(), cclock);\
		}}

	#define TOCK(str,size)  {if (print_kernel_timing) {\
		host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);\
		clock_get_time(cclock, &tock_clockData);\
		mach_port_deallocate(mach_task_self(), cclock);\
		std::cerr<< str << " ";\
		if((tock_clockData.tv_sec > tick_clockData.tv_sec) && (tock_clockData.tv_nsec >= tick_clockData.tv_nsec))   std::cerr<< tock_clockData.tv_sec - tick_clockData.tv_sec << std::setfill('0') << std::setw(9);\
		std::cerr  << (( tock_clockData.tv_nsec - tick_clockData.tv_nsec) + ((tock_clockData.tv_nsec<tick_clockData.tv_nsec)?1000000000:0)) << " " <<  size << std::endl;}}
#else
	
	#define TICK()    {if (print_kernel_timing) {clock_gettime(CLOCK_MONOTONIC, &tick_clockData);}}

	#define TOCK(str,size)  {if (print_kernel_timing) {clock_gettime(CLOCK_MONOTONIC, &tock_clockData); std::cerr<< str << " ";\
		if((tock_clockData.tv_sec > tick_clockData.tv_sec) && (tock_clockData.tv_nsec >= tick_clockData.tv_nsec))   std::cerr<< tock_clockData.tv_sec - tick_clockData.tv_sec << std::setfill('0') << std::setw(9);\
		std::cerr  << (( tock_clockData.tv_nsec - tick_clockData.tv_nsec) + ((tock_clockData.tv_nsec<tick_clockData.tv_nsec)?1000000000:0)) << " " <<  size << std::endl;}}

#endif

#ifdef debug
#undef debug
#endif
#ifdef string
#undef string
#endif

#ifndef DEBUG
#define DEBUG 1
#endif

#if DEBUG != 0
#define string(a) #a
#define debug(var) std::cout << "[" << __FILE__ << ":" << __FUN ## CTION__ << ":" << __LINE__ << "] " << string(var) << " = \"" << (var) << "\"" << std::endl;
#else
#define debug(var)
#endif

#define checkVolume() { float volumePreTotal = 0; for(uint x = 0; x < volume.size.x; x++) { for(uint y = 0; y < volume.size.y; y++) { for(uint z = 0; z < volume.size.z; z++) { volumePreTotal += volume[make_uint3(x, y, z)].x; volumePreTotal += volume[make_uint3(x, y, z)].y; } } } debug(volumePreTotal); }

drake_declare(drake_kfusion);

// inter-frame
Volume volume;

float *gaussian;

bool print_kernel_timing = false;
#ifdef __APPLE__
	clock_serv_t cclock;
	mach_timespec_t tick_clockData;
	mach_timespec_t tock_clockData;
#else
	struct timespec tick_clockData;
	struct timespec tock_clockData;
#endif

static drake_platform_t stream;
static drake_kfusion_t drake_init;
bool preprocessing_ok;
bool tracking_ok;
bool integration_ok;
bool raycasting_ok;
bool pipeline_is_running;
	
static
void
matrix4ToFloat16(float out[16], const Matrix4 &in)
{
	out[0] = in.data[0].x;
	out[1] = in.data[0].y;
	out[2] = in.data[0].z;
	out[3] = in.data[0].w;
	out[4] = in.data[1].x;
	out[5] = in.data[1].y;
	out[6] = in.data[1].z;
	out[7] = in.data[1].w;
	out[8] = in.data[2].x;
	out[9] = in.data[2].y;
	out[10] = in.data[2].z;
	out[11] = in.data[2].w;
	out[12] = in.data[3].x;
	out[13] = in.data[3].y;
	out[14] = in.data[3].z;
	out[15] = in.data[3].w;
}

void
Kfusion::languageSpecificConstructor()
{
	ia_arguments_t args;
	args.num_cores = drake_application_number_of_cores(drake_kfusion); 
	args.poll_at_idle = 1;
	args.wait_after_scaling = 0;
	stream = drake_platform_init(&args);

	// ********* BEGIN : Generate the gaussian *************
	size_t gaussianS = radius * 2 + 1;
	gaussian = (float*) calloc(gaussianS * sizeof(float), 1);
	int x;
	for (unsigned int i = 0; i < gaussianS; i++) {
		x = i - 2;
		gaussian[i] = expf(-(x * x) / (2 * delta * delta));
	}
	// ********* END : Generate the gaussian *************

	// Gather kfusion parameters
	drake_init.compSize = computationSize;
	drake_init.gaussian = gaussian; 
	drake_init.e_d = e_delta;
	drake_init.r = radius;
	drake_init.dist_threshold = dist_threshold;
	drake_init.normal_threshold = normal_threshold;
	drake_init.maxweight = maxweight;
	drake_init.track_threshold = track_threshold;
	drake_init.volume = &volume;
	drake_init.nearPlane = nearPlane;
	drake_init.farPlane = farPlane;
	drake_init.step = min(this->volumeDimensions) / max(this->volumeResolution);
	size_t buffer_size = 0;
	drake_init.pyramid = this->iterations;
	
	drake_init.pose = this->pose;

	drake_init.viewPose = &this->viewPose;
	drake_init.light = light;
	drake_init.ambient = ambient;

	// Create stream
	drake_platform_stream_create(stream, drake_kfusion);
	pipeline_is_running = false;
	this->reset();
}

static void start_pipeline()
{
	if(
		   !pipeline_is_running
	)
	{
		// To be run iff all parameters required have been gathered 
		drake_platform_stream_run_async(stream);
		pipeline_is_running = true;
	}
}

static void stop_pipeline()
{
	if(pipeline_is_running)
	{
		drake_init.no_more_frame = 1;
		sem_post(&drake_init.frame_queue);
		drake_platform_stream_wait(stream);
		pipeline_is_running = false;
	}
}

static
void
process_frame()
{
	start_pipeline();
	
	// Unlock top task in pipeline
	if(
		   pipeline_is_running
		&& preprocessing_ok
		&& tracking_ok
		&& integration_ok
		&& raycasting_ok
	)
	{
		// Make first task to consume frame
		sem_post(&drake_init.frame_queue);
		// Wait for frame to be consumed, but not the frame to be processed
		sem_wait(&drake_init.frame_consume);
		// Wait for the frame to be processed
		sem_wait(&drake_init.frame_output);

		// Wait for depth rendering to be done, if enabled
		if(drake_init.render_depth != 0)
		{
			sem_wait(&drake_init.render_depth_done);
		}
		// Wait for track rendering to be done, if enabled
		if(drake_init.render_track != 0)
		{
			sem_wait(&drake_init.render_track_done);
		}
		// Wait for volume rendering to be done, if enabled
		if(drake_init.render_volume != 0)
		{
			sem_wait(&drake_init.render_volume_done);
		}

		preprocessing_ok = false;
		tracking_ok = false;
		integration_ok = false;
		raycasting_ok = false;
	}
}

Kfusion::~Kfusion()
{
	// Stop pipeline first
	stop_pipeline();

	// Then destroy it and free memory
	drake_platform_stream_destroy(stream);
	drake_platform_destroy(stream);
}

void
Kfusion::reset()
{
	// Stop pipeline if running
	stop_pipeline();

	// Claim prerequesites as not met
	preprocessing_ok = false;
	tracking_ok = false;
	integration_ok = false;
	raycasting_ok = false;
	drake_init.render_depth = 0;
	drake_init.render_track = 0;
	drake_init.render_volume = 0;

	// Reninitialize frame lock
	//drake_init.frame_done = 1;
	drake_init.no_more_frame = 0;

	// Reinitialize tasks
	drake_platform_stream_init(stream, &drake_init);

	// Reinitialize volume
	volume.init(volumeResolution, volumeDimensions);
	initVolumeKernel(volume);

	// Initialize host-to-app synchronization
	sem_init(&drake_init.frame_queue, 0, 0);
	sem_init(&drake_init.frame_consume, 0, 0);
	sem_init(&drake_init.frame_output, 0, 0);
}

void
init()
{
}

void
clean()
{
}

void
initVolumeKernel(Volume volume)
{
	for (unsigned int x = 0; x < volume.size.x; x++)
	{
		for (unsigned int y = 0; y < volume.size.y; y++)
		{
			for (unsigned int z = 0; z < volume.size.z; z++)
			{
				volume.setints(x, y, z, make_float2(1.0f, 0.0f));
			}
		}
	}
}


bool
Kfusion::preprocessing(const ushort *inputDepth, const uint2 inputSize)
{
	drake_init.in = inputDepth;
	drake_init.inSize = inputSize;

	preprocessing_ok = true;
	process_frame();

	return true;
}

bool
Kfusion::tracking(float4 k, float icp_threshold, uint tracking_rate, uint frame)
{
	drake_init.k = k;
	drake_init.icp_threshold = icp_threshold;
	drake_init.tracking_rate = tracking_rate;

	tracking_ok = true;
	process_frame();

	return true;
}


bool
Kfusion::integration(float4 k, uint integration_rate, float mu, uint frame)
{
	drake_init.integration_rate = integration_rate;
	drake_init.mu = mu;

	integration_ok = true;
	process_frame();

	return true;
}

bool
Kfusion::raycasting(float4 k, float mu, uint frame)
{
	raycasting_ok = true;
	process_frame();

	return true;
}

void
Kfusion::dumpVolume(const char *filename)
{
	std::ofstream fDumpFile;

	if (filename == NULL) {
		return;
	}

	std::cout << "Dumping the volumetric representation on file: " << filename
			<< std::endl;
	fDumpFile.open(filename, std::ios::out | std::ios::binary);
	if (fDumpFile.fail()) {
		std::cout << "Error opening file: " << filename << std::endl;
		exit(1);
	}

	// Dump on file without the y component of the short2 variable
	for (unsigned int i = 0; i < volume.size.x * volume.size.y * volume.size.z;
			i++) {
		fDumpFile.write((char *) (volume.data + i), sizeof(short));
	}

	fDumpFile.close();
}

void
Kfusion::renderDepth(uchar4 * out, uint2 outputSize)
{
	drake_init.render_depth_buffer = out;
	drake_init.render_depth = 1;
	process_frame();
}

void
Kfusion::renderTrack(uchar4 * out, uint2 outputSize)
{
	drake_init.render_track_buffer = out;
	drake_init.render_track = 1;
	process_frame();
}

void
Kfusion::renderVolume(uchar4 * out, uint2 outputSize, int frame, int raycast_rendering_rate, float4 k, float largestep)
{
	drake_init.rendering_rate = raycast_rendering_rate;
	drake_init.largestep = largestep;
	drake_init.render_volume_buffer = out;

	drake_init.render_volume = 1;
	process_frame();
}

void
Kfusion::computeFrame(const ushort * inputDepth, const uint2 inputSize, float4 k, uint integration_rate, uint tracking_rate, float icp_threshold, float mu, const uint frame)
{
	// Preprocessing
	drake_init.in = inputDepth;
	drake_init.inSize = inputSize;
	preprocessing_ok = true;

	// Tracking
	drake_init.k = k;
	drake_init.icp_threshold = icp_threshold;
	drake_init.tracking_rate = tracking_rate;
	tracking_ok = true;
	
	// Integration
	drake_init.integration_rate = integration_rate;
	drake_init.mu = mu;
	integration_ok = true;

	//Raycasting
	raycasting_ok = true;

	// Don't care about frame number
	
	// Trigger frame processing
	process_frame();
}

void
synchroniseDevices()
{
	// Nothing to do in the C++/Drake implementation
}

