#include <axon.h>
#include <axon/log.h>
#include <axon/util.h>

int main()
{
	axon::log lg;
	lg.open("./test.log");
	lg<<"this is a test >"<<1.23<<std::endl;
}