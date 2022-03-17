#pragma once

class Mmf
{
public:
	Mmf()
		: m_handle(nullptr)
		, m_pointer(nullptr)
	{
	}

	~Mmf() = default;

	int Open(const char* t_name);
	void Write(const void* t_data, const size_t& t_size);
	void Close();

private:
	int Open();
	int Create(const char* t_name);

	void* m_handle;
	void* m_pointer;

};