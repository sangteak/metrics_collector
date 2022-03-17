#pragma once

#include <thread>
#include <mutex>
#include <array>
#include "perf_interface.h"
#include "mmf.h"

////////////////////////////////////////////////////////////////////////////////
// 작성자: 누군가
// 설  명: 공유 메모리를 이용해 perfmon과 연동하기 위해 제작됨. (지원 플랫폼 : Windows)
// 
// 1) 수집을 위한 리파지토리 구성(여러개 구성 가능)
// 2) 생성한 리파지토리 파싱 및 마샬링을 위해 IPerfService 상속 구현
// 3) 수집기를 선언 후 시작
// 4) 필요한 위치에서 PerfTask에 수집 데이터 저장 후 수집기로 전달
//
// ** 공유 메모리에 주기적으로 데이터를 갱신하기 때문에 perfmon 또는 기타 툴을 이용해 해당 데이터를 읽어 기록해야함.

/*
1) 서비스 생성
class PerfService : public IPerfService
{
public:
	enum eId : int32_t
	{
		SEND_PACKET = 1,
		SEND_PACKET_AMOUNT = 2,
	};

	virtual bool Collect(const int32_t& id, const std::shared_ptr<PerfTask>& msg)
	{
		switch (id)
		{
		case eId::SEND_PACKET:
			++m_sendPacketCount;
			break;

		case eId::SEND_PACKET_AMOUNT:
			m_sendPacketAmount += msg->get<0>();
			break;

		default:
			return false;
		}

		return true;
	}

	virtual bool Marshel(std::vector<int32_t>& result)
	{
		result.emplace_back(m_sendPacketCount);
		result.emplace_back(m_sendPacketAmount);
	}

	virtual void Reset()
	{
		m_sendPacketCount = 0;
		m_sendPacketAmount = 0;
	}

private:
	int32_t m_sendPacketCount = 0;
	int32_t m_sendPacketAmount = 0;
};

2) PerfExecutor 생성 및 시작

// 객체 선언
nmsp::perf_collector g_collector;

void main(void)
{
	PerfService service;
	PerfExecutor perfExecutor;
	perfExecutor.Start("Global\\sample_mmf", 
	// 수집기 시작!
	if (false == g_collector.Start("sample_mmf", std::static_pointer_cast<IPerfService>(std::make_shared<PerfService>()), 1000, PerfExecutor::eThreadType::WORKER_THREAD)
	{
		return;
	}

	// 주기적 데이터 입력을 위한 쓰레드 생성
	auto producer = std::thread([&perfExecutor]() {

		nmsp::_stop_watch_t endTimer;

		while ((3600 * 1000) > endTimer.Lap())
		{
		// 수집기에 등록
			perfExecutor.Put(PerfService::eId::SEND_PACKET_AMOUNT, std::make_shared<PerfTask>(1000)));

			std::this_thread::sleep(std::chrono::milliseconds(10);
		}
	});

	producer.join();

	std::this_thread::sleep(std::chrono::seconds(30);

	std::cout << "terminate" << std::endl;

	getchar();
}
*/

//#include <tbb/concurrent_queue.h>

// binary 구성을 위한 streambuf
// 사용법은 boost::asio::streambuf 문서 참고
// class perf_result : public boost::asio::streambuf
// {
// public:
// 	using __super_t = boost::asio::streambuf;
// 
// 	const void* get_pointer()
// 	{
// 		return boost::asio::buffer_cast<const void*>(__super_t::data());
// 	}
// };
template <typename Queue>
class PerfExecutor
{
	static_assert(std::is_base_of_v<IPerfQueue, Queue>, "A Queue had not inherit a IPerfQueue class.");

public:
	static const int32_t MMF_WRITE_DEFAULT_INTERVAL_MS = 1000;

	using _key_t = int32_t;
	using _value_t = _task_pack_t;
	using _element_t = std::tuple<_key_t, _value_t>; // first : 구분 ID, second : 데이터(Repository 또는 원하는 포멧)
	using _channel_t = std::shared_ptr<IPerfQueue>;
	using _service_t = std::shared_ptr<IPerfService>;
	using _result_t = std::vector<int32_t>;
	using _mmf_t = Mmf;

	enum class eThreadType
	{
		NONE = 0,
		WORKER_THREAD,
	};

	PerfExecutor()
		: m_isRun(false)
		, m_nextTime(0)
		, m_collectionIntervalMs(0)
		, m_service(nullptr)
		, m_channel(new Queue)
	{
	}
	
	// @fn bool Start(const std::string& t_mmfName, _service_t& t_service,	int32_t t_collectionIntervalMs,	eThreadType t_threadType)
	// @brief 
	// @return bool 
	// @param t_mmfName 공유 메모리에 사용할 이름(perfmon 및 기타 툴에서 조회를 위한 이름)
	// @param t_service 사용자가 정의한 서비스 객체
	// @param t_collectionIntervalMs 공유 메모리 적재 간격
	// @param t_threadType NONE(쓰레드 사용 안함), WOREKR_THREAD(별도 쓰레드 생성)
	bool Start(const char* t_mmfName, _service_t& t_service, int32_t t_collectionIntervalMs, eThreadType t_threadType)
	{
		if (NO_ERROR != m_mmf.Open(t_mmfName))
			return false;

		m_service = t_service;

		m_collectionIntervalMs = t_collectionIntervalMs;
		if (0 >= m_collectionIntervalMs)
			m_collectionIntervalMs = MMF_WRITE_DEFAULT_INTERVAL_MS;

		m_isRun = true;
		m_threadType = t_threadType;

		if (eThreadType::WORKER_THREAD == m_threadType)
		{
			m_workerTh = std::thread([this]() {

				using namespace std::chrono;

				while (true == m_isRun)
				{
					Collect(std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

					std::this_thread::sleep_for(std::chrono::milliseconds(m_collectionIntervalMs));
				}

				// 만일 쓰레드가 비정상적으로 종료됐을 경우 데이터가 쌓이는걸 방지하기 위해 실행 상태를 false로 변경한다.
				m_isRun = false;
			});
		}

		return true;
	}

	void Stop()
	{
		m_isRun = false;

		if (true == m_workerTh.joinable())
		{
			m_workerTh.join();
			m_threadType = eThreadType::NONE;
		}

		m_mmf.Close();
	}

	// immediate를 이용할 경우 데이터 손실이 있을 수 있음.
	//void Put(const _value_t& value, bool immediate = false)
	void Put(const _key_t& key, _value_t&& value)
	{
		if (false == m_isRun)
			return;

		m_channel->Put(std::move(std::make_tuple(key, std::forward<_value_t>(value))));
 		
	}

	void Collect(const uint64_t now)
	{
		if (false == m_isRun)
			return;
		
		while (true == m_isRun)
		{
			_element_t task;
			if (false == m_channel->TryPop(task))
				break;

			m_service->Collect(std::get<0>(task), std::move(std::get<1>(task)));
		}

		if (0 == m_nextTime)
		{
			m_nextTime = now + m_collectionIntervalMs;
			return;
		}

		// 일정 시간이 흘렀다면?
		if (now >= m_nextTime)
		{
			m_service->Marshel(m_result);
			m_service->Reset();

			m_mmf.Write(m_result.data(), m_result.size());

			// mmf에 쓰기 완료됐으므로 초기화합니다.
			m_result.clear();
			
			m_nextTime = now + m_collectionIntervalMs;
		}
	}

private:
	std::atomic_bool m_isRun;

	eThreadType m_threadType;

	uint64_t m_nextTime;

	uint64_t m_collectionIntervalMs;

	// 데이터 수집 채널
	_channel_t m_channel;

	// 일정 시간동안 쌓인 데이터를 분석&정리 합니다.
	_service_t m_service;

	// 분석한 결과를 공유 메모리에 적재합니다.
	_mmf_t m_mmf;

	// Write Buffer
	_result_t m_result;

	// 워커 쓰레드 핸들러
	std::thread m_workerTh;
};