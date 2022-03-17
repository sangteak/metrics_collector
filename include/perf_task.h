#pragma once

template <typename Element, int MaxArguments>
class PerfTask
{
public:
	PerfTask()
	{
		m_data.fill(0);
	}

	template <typename... Args>
	explicit PerfTask(Args... args)
		: PerfTask()
	{
		static_assert(MaxArguments >= std::tuple_size<std::tuple<Args...>>{}, "The argument size is over MaxArguments.");
		static_assert(std::disjunction<std::is_same<Element, Args>...>::value, "Wrong the Args type.");

		Packed(std::make_tuple(args...), std::index_sequence_for<Args...>{});
	}

	~PerfTask() = default;

	template <int32_t N>
	constexpr auto get()
	{
		static_assert(0 <= N && MaxArguments > N, "Wrong a N.");
		return m_data.at(N);
	}

private:
	template <typename Tuple, std::size_t... Is>
	void Packed(const Tuple& t, std::index_sequence<Is...>)
	{
		((m_data[Is] = std::get<Is>(t)), ...);
	}

private:
	std::array<Element, MaxArguments> m_data;
};
