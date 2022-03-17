#pragma once

#include <thread>
#include <mutex>
#include <array>
#include "perf_interface.h"
#include "mmf.h"

////////////////////////////////////////////////////////////////////////////////
// �ۼ���: ������
// ��  ��: ���� �޸𸮸� �̿��� perfmon�� �����ϱ� ���� ���۵�. (���� �÷��� : Windows)
// 
// 1) ������ ���� �������丮 ����(������ ���� ����)
// 2) ������ �������丮 �Ľ� �� �������� ���� IPerfService ��� ����
// 3) �����⸦ ���� �� ����
// 4) �ʿ��� ��ġ���� PerfTask�� ���� ������ ���� �� ������� ����
//
// ** ���� �޸𸮿� �ֱ������� �����͸� �����ϱ� ������ perfmon �Ǵ� ��Ÿ ���� �̿��� �ش� �����͸� �о� ����ؾ���.

/*
1) ���� ����
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

2) PerfExecutor ���� �� ����

// ��ü ����
nmsp::perf_collector g_collector;

void main(void)
{
	PerfService service;
	PerfExecutor perfExecutor;
	perfExecutor.Start("Global\\sample_mmf", 
	// ������ ����!
	if (false == g_collector.Start("sample_mmf", std::static_pointer_cast<IPerfService>(std::make_shared<PerfService>()), 1000, PerfExecutor::eThreadType::WORKER_THREAD)
	{
		return;
	}

	// �ֱ��� ������ �Է��� ���� ������ ����
	auto producer = std::thread([&perfExecutor]() {

		nmsp::_stop_watch_t endTimer;

		while ((3600 * 1000) > endTimer.Lap())
		{
		// �����⿡ ���
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

// binary ������ ���� streambuf
// ������ boost::asio::streambuf ���� ����
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
	using _element_t = std::tuple<_key_t, _value_t>; // first : ���� ID, second : ������(Repository �Ǵ� ���ϴ� ����)
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
	// @param t_mmfName ���� �޸𸮿� ����� �̸�(perfmon �� ��Ÿ ������ ��ȸ�� ���� �̸�)
	// @param t_service ����ڰ� ������ ���� ��ü
	// @param t_collectionIntervalMs ���� �޸� ���� ����
	// @param t_threadType NONE(������ ��� ����), WOREKR_THREAD(���� ������ ����)
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

				// ���� �����尡 ������������ ������� ��� �����Ͱ� ���̴°� �����ϱ� ���� ���� ���¸� false�� �����Ѵ�.
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

	// immediate�� �̿��� ��� ������ �ս��� ���� �� ����.
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

		// ���� �ð��� �귶�ٸ�?
		if (now >= m_nextTime)
		{
			m_service->Marshel(m_result);
			m_service->Reset();

			m_mmf.Write(m_result.data(), m_result.size());

			// mmf�� ���� �Ϸ�����Ƿ� �ʱ�ȭ�մϴ�.
			m_result.clear();
			
			m_nextTime = now + m_collectionIntervalMs;
		}
	}

private:
	std::atomic_bool m_isRun;

	eThreadType m_threadType;

	uint64_t m_nextTime;

	uint64_t m_collectionIntervalMs;

	// ������ ���� ä��
	_channel_t m_channel;

	// ���� �ð����� ���� �����͸� �м�&���� �մϴ�.
	_service_t m_service;

	// �м��� ����� ���� �޸𸮿� �����մϴ�.
	_mmf_t m_mmf;

	// Write Buffer
	_result_t m_result;

	// ��Ŀ ������ �ڵ鷯
	std::thread m_workerTh;
};