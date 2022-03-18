# METRICS PRODUCER
서버 성능 모니터링을 위한 메트릭 수집기 입니다.

# 특징
- perfmon에 사용자 메트릭을 추가하기 위해 작성
- metrics::IService를 상속하여 메트릭 수집 및 결과 보고 기능 작성
- metrics::Producer는 정해진 시간에 metrics::IService로 부터 받은 결과를 공유 메모리에 적재

# 빌드
- C++17 또는 최신 컴파일러 필요
- cmake 3.0 이상 버전 필요
- cmake 빌드(Visual Studio 2019 기준)
```
> cmake -G 를 사용하여 Generators 확인
> cmake ./ -G "Visual Studio 16 2019"
> 솔루션 파일(.sln) 오픈하여 빌드
```

# 사용 예
```cpp
#include <iostream>
#include <deque>
#include <mutex>
#include <chrono>
#include "metrics_producer.h"

// Thread-Safe 한 Queue가 필요하며 metrics::Producer는 metrics::IQueue 인터페이스를 상속받은 Queue를 요구합니다.
class MetricsQueue : public metrics::IQueue
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

// 메트릭을 수집할 Service 객체를 구현합니다.
class MetricsService : public metrics::IService
{
public:
  // Metric ID
	enum eId : int32_t
	{
		SEND_PACKET = 1,
		SEND_PACKET_AMOUNT = 2,
	};

  // 데이터 수집
	virtual bool Collect(const int32_t& id, metrics::_task_pack_t&& task) override
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

  // 결과 반환
	virtual bool Marshel(std::vector<int32_t>& result) override
	{
		result.emplace_back(m_sendPacketCount);
		result.emplace_back(m_sendPacketAmount);

		return true;
	}

  // metrics::Producer에서 Marshel 호출 후 
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
	using _metrics_producer_t = metrics::Producer<MetricsQueue>;
	
  // 서비스 객체 생성.
  // producer에 넘기기위해 metrics::IService로 형변환
	auto service = std::static_pointer_cast<metrics::IService>(std::make_shared<MetricsService>());

  // producer 객체 생성
	_metrics_producer_t producer;
  
  // 수집 시작
	producer.Run(
    // 공유 메모리 이름. Global\\[이름] 형식으로 지정 필요함.
		"Global\\sample_mmf",
    
    // 서비스 등록
		service,
    
    // 공유 메모리 적재 간격 지정. (1000 ms)
		1000,
    
    // 쓰레드 타입 지정. 
    // - _metrics_producer_t::eThreadType::NONE : 유저가 쓰레드를 관리하게 되며 주기적으로 proceduer.Collect(now) 메서드 호출 필요.
    // - _metrics_producer_t::eThreadType::WORKER_THREAD : 워커 쓰레드 생성하여 주기적으로 공유 메모리에 적재
    _metrics_producer_t::eThreadType::WORKER_THREAD
	);

	// 데이터 생성을 위핸 쓰레드 
	auto producerWorkerThreadHandler = std::thread([&producer]() {

		auto startTime = std::chrono::steady_clock::now();
		auto endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
  
    // 1초에 한번씩 6개의 데이터 수집을 위한 루프
		while (6000 > endTime)
		{
      // 메트릭 수집용 task 생성
      // vardict template으로 int형 등록 가능
			metrics::_task_pack_t task(1000);

			// 수집기에 등록
			producer.Put(MetricsService::eId::SEND_PACKET_AMOUNT, std::move(task));

			std::this_thread::sleep_for(std::chrono::seconds(1));
			endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
		}
	});

	producerWorkerThreadHandler.join();

  // 데이터 생성을 위한 쓰레드 종료 후 3초 대기
	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << "terminate" << std::endl;

	producer.Stop();

	getchar();
	
	return 0;
}
```

