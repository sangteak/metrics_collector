#pragma once

#include <string>
#include <vector>
#include "perf_task.h"

using _task_pack_t = PerfTask<int, 10>;

class IPerfService
{
public:
	virtual bool Collect(const int32_t& id, _task_pack_t&& task) = 0;
	virtual bool Marshel(std::vector<int32_t>& result) = 0;
	virtual void Reset() = 0;
};

class IPerfQueue
{
public:
	using _key_t = int;
	using _value_t = _task_pack_t;
	using _element_t = std::tuple<_key_t, _value_t>;

	virtual bool TryPop(_element_t& task) = 0;
	virtual void Put(_element_t&& task) = 0;
};