﻿#include <iostream>
#include <string>
#include <codecvt>
#include <cassert>
#include <deque>
#include <locale>
#include <unordered_map>
#include <windows.h>
#include <chrono>
#include "perf_executor.h"

class PerfQueue : public IPerfQueue
{
public:
	virtual bool TryPop(_element_t& task)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (true == m_queue.empty())
		{
			return false;
		}

		task = m_queue.front();
		m_queue.pop_front();

		return true;
	}

	virtual void Put(_element_t&& task)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push_back(std::forward<_element_t>(task));
	}

private:
	std::mutex m_mutex;
	std::deque<_element_t> m_queue;
};

class PerfService : public IPerfService
{
public:
	enum eId : int32_t
	{
		SEND_PACKET = 1,
		SEND_PACKET_AMOUNT = 2,
	};

	virtual bool Collect(const int32_t& id, _task_pack_t&& task) override
	{
		switch (id)
		{
		case eId::SEND_PACKET:
			++m_sendPacketCount;
			break;

		case eId::SEND_PACKET_AMOUNT:
			m_sendPacketAmount += task.get<0>();
			break;

		default:
			return false;
		}

		std::cout << "count=" << m_sendPacketCount << ", amount=" << m_sendPacketAmount << std::endl;
	
		return true;
	}

	virtual bool Marshel(std::vector<int32_t>& result) override
	{
		result.emplace_back(m_sendPacketCount);
		result.emplace_back(m_sendPacketAmount);

		return true;
	}

	virtual void Reset() override
	{
		m_sendPacketCount = 0;
		m_sendPacketAmount = 0;
	}

private:
	int32_t m_sendPacketCount = 0;
	int32_t m_sendPacketAmount = 0;
};

int main() 
{	
	using _perf_executor_t = PerfExecutor<PerfQueue>;

	_perf_executor_t perfExecutor;
	perfExecutor.Run(
		"Global\\sample_mmf",
		std::static_pointer_cast<IPerfService>(std::make_shared<PerfService>()),
		1000,
		_perf_executor_t::eThreadType::WORKER_THREAD
	);

	// 주기적 데이터 입력을 위한 쓰레드 생성
	auto producer = std::thread([&perfExecutor]() {

		auto startTime = std::chrono::steady_clock::now();
		auto endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();

		while (6000 > endTime)
		{
			_task_pack_t task(1000);

			// 수집기에 등록
			perfExecutor.Put(PerfService::eId::SEND_PACKET_AMOUNT, std::move(task));

			std::this_thread::sleep_for(std::chrono::seconds(1));

			endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
		}
	});

	producer.join();

	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << "terminate" << std::endl;

	perfExecutor.Stop();

	getchar();
	
	return 0;
}
