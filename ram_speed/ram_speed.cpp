﻿//  -----------------------------------------------------------------------------------------
//    ram_speed by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  -----------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <process.h>
#pragma comment(lib, "winmm.lib")

#include "cpu_info.h"
#include "simd_util.h"
#include "ram_speed.h"

typedef struct {
	int mode;
	uint32_t check_size_bytes;
	uint32_t thread_id;
	double megabytes_per_sec;
} RAM_SPEED_THREAD;

#ifdef __cplusplus
extern "C" {
#endif
extern void __stdcall read_sse(uint8_t *src, uint32_t size, uint32_t count_n);
extern void __stdcall read_avx(uint8_t *src, uint32_t size, uint32_t count_n);
extern void __stdcall write_sse(uint8_t *dst, uint32_t size, uint32_t count_n);
extern void __stdcall write_avx(uint8_t *dst, uint32_t size, uint32_t count_n);
#ifdef __cplusplus
}
#endif


typedef void(__stdcall *func_ram_test)(uint8_t *dst, uint32_t size, uint32_t count_n);

unsigned int __stdcall ram_speed_func(void *prm) {
	const int TEST_COUNT = 4;

	RAM_SPEED_THREAD *thread_prm = (RAM_SPEED_THREAD *)prm;

	uint32_t check_size_bytes = (thread_prm->check_size_bytes + 255) & ~255;
	const uint32_t test_kilo_bytes   = (uint32_t)(16 * 1024 * 1024 / std::max(1.0, log2(check_size_bytes / 1024.0)) + 0.5);
	const uint32_t warmup_kilo_bytes = test_kilo_bytes * 2;
	uint8_t *ptr = (uint8_t *)_aligned_malloc(check_size_bytes, 32);
	uint32_t count_n = (int)(test_kilo_bytes * 1024.0 / check_size_bytes + 0.5);
	int avx = 0 != (get_availableSIMD() & AVX);
	int64_t result[TEST_COUNT];
	static const func_ram_test RAM_TEST_LIST[][2] = {
		{read_sse, write_sse},
		{read_avx, write_avx},
	};

	const func_ram_test ram_test = RAM_TEST_LIST[avx][thread_prm->mode];
	
	int64_t start, fin, freq;
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
	ram_test(ptr, check_size_bytes, (int)(warmup_kilo_bytes * 1024.0 / check_size_bytes + 0.5));
	for (int i = 0; i < TEST_COUNT; i++) {
		QueryPerformanceCounter((LARGE_INTEGER *)&start);
		ram_test(ptr, check_size_bytes, count_n);
		QueryPerformanceCounter((LARGE_INTEGER *)&fin);
		result[i] = fin - start;
	}
	ram_test(ptr, check_size_bytes, (int)(warmup_kilo_bytes * 1024.0 / check_size_bytes + 0.5));
	_aligned_free(ptr);

	int64_t time_min = INT64_MAX;
	for (int i = 0; i < TEST_COUNT; i++)
		time_min = std::min(time_min, result[i]);

	double time_ms = time_min * 1000.0 / freq;
	thread_prm->megabytes_per_sec = (check_size_bytes * (double)count_n / (1024.0 * 1024.0)) / (time_ms * 0.001);
	_endthreadex(0);
	return 0;
}

double ram_speed_mt(int check_size_kilobytes, int mode, int thread_n) {
	std::vector<HANDLE> threads(thread_n, NULL);
	std::vector<RAM_SPEED_THREAD> thread_prm(thread_n);
	DWORD physical_processor_core = 0, logical_processor_core = 0;
	getProcessorCount(&physical_processor_core, &logical_processor_core);
	for (uint32_t i = 0; i < threads.size(); i++) {
		thread_prm[i].mode = (mode == RAM_SPEED_MODE_RW) ? (i & 1) : mode;
		thread_prm[i].check_size_bytes = (check_size_kilobytes * 1024 / thread_n + 255) & ~255;
		thread_prm[i].thread_id = (i % physical_processor_core) * (logical_processor_core / physical_processor_core) + (int)(i / physical_processor_core);
		threads[i] = (HANDLE)_beginthreadex(NULL, 0, ram_speed_func, &thread_prm[i], CREATE_SUSPENDED, NULL);
		//渡されたスレッドIDからスレッドAffinityを決定
		//特定のコアにスレッドを縛り付ける
		SetThreadAffinityMask(threads[i], 1 << (int)thread_prm[i].thread_id);
		//高優先度で実行
		SetThreadPriority(threads[i], THREAD_PRIORITY_HIGHEST);
	}
	
	for (uint32_t i = 0; i < thread_prm.size(); i++) {
		ResumeThread(threads[i]);
	}
	WaitForMultipleObjects((DWORD)threads.size(), &threads[0], TRUE, INFINITE);
	for (auto th : threads) {
		if (th) {
			CloseHandle(th);
		}
	}

	double sum = 0.0;
	for (const auto& prm : thread_prm) {
		sum += prm.megabytes_per_sec;
	}
	return sum;
}

std::vector<double> ram_speed_mt_list(int check_size_kilobytes, int mode) {
	DWORD physical_processor_core = 0, logical_processor_core = 0;
	getProcessorCount(&physical_processor_core, &logical_processor_core);

	std::vector<double> results;
	for (uint32_t ith = 1; ith <= physical_processor_core; ith++) {
		results.push_back(ram_speed_mt(check_size_kilobytes, mode, ith));
	}
	return results;
}

int main(int argc, char **argv) {
	char mes[256];
	getCPUInfo(mes, 256);
	fprintf(stderr, "%s\r\n", mes);
	FILE *fp = NULL;
	if (fopen_s(&fp, "result.csv", "wb") || NULL == fp) {
		fprintf(stderr, "failed to open output file.\n");
	} else {
		fprintf(fp, "%s\r\n", mes);
		fprintf(fp, "read\r\n");
		for (int i_size = 1; i_size <= 17; i_size++) {
			const int size_in_kilo_byte = 1 << i_size;
			fprintf(fp, "%6d KB,", size_in_kilo_byte);
			std::vector<double> results = ram_speed_mt_list(size_in_kilo_byte, RAM_SPEED_MODE_READ);
			for (uint32_t i = 0; i < results.size(); i++) {
				fprintf(fp, "%6.1f,", results[i] / 1024.0);
				fprintf(stderr, "%6d KB, %2d threads: %6.1f GB/s\n", size_in_kilo_byte, i+1, results[i] / 1024.0);
			}
			fprintf(fp, "\r\n");
		}
		fprintf(fp, "\r\n");
		fprintf(fp, "write\r\n");
		for (int i_size = 1; i_size <= 17; i_size++) {
			const int size_in_kilo_byte = 1 << i_size;
			fprintf(fp, "%6d KB,", size_in_kilo_byte);
			std::vector<double> results = ram_speed_mt_list(size_in_kilo_byte, RAM_SPEED_MODE_WRITE);
			for (uint32_t i = 0; i < results.size(); i++) {
				fprintf(fp, "%6.1f,", results[i] / 1024.0);
				fprintf(stderr, "%6d KB, %2d threads: %6.1f GB/s\n", size_in_kilo_byte, i+1, results[i] / 1024.0);
			}
			fprintf(fp, "\r\n");
		}
		fclose(fp);
	}
}

