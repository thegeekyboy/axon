#include <iostream>
#include <streambuf>

class buffer: public std::streambuf {

	std::streambuf *sb;

	public:

	using char_type = typename std::basic_streambuf<char>::char_type;
	using int_type = typename std::basic_streambuf<int>::int_type;

	int_type overflow(int_type ch) { return std::basic_streambuf<char>::overflow(ch); };

	traits_type::int_type underflow()
	{
		traits_type::int_type i;

		while ((i = sb->sbumpc()) == '\0');

		if (!traits_type::eq_int_type(i, traits_type::eof()))
		{
			char ch = traits_type::to_char_type(i);
			setg(&ch, &ch, &ch+1);
		}

		return i;
	}

	void print() { };
};

int main([[maybe_unused]]int argv, [[maybe_unused]]char *argc[])
{
	buffer buf;
	std::ostream y(&buf);

	
	y<<123;
	y<<"Asd";

	std::cout<<y.rdbuf();

	return 0;
}