#include <cstdio>
#include <string>
#include <stdexcept>

#include <pfq.hpp>
using namespace net;

int
main(int argc, char *argv[])
{
    if (argc < 2)
        throw std::runtime_error(std::string("usage: ").append(argv[0]).append(" dev"));
    
    pfq r(1514);

    r.bind(argv[1], pfq::any_queue);

    r.timestamp_enabled(true);
    
    r.enable();
    
    for(;;)
    {
            auto many = r.read( 1000000 /* timeout: micro */);
            
            std::cout << "batch size: " << many.size() << " ===>" << std::endl;

            for(auto & packet : many)
            {       
                char *buff;
                while(!(buff = static_cast<char *>(data_ready(packet, many.index()))))
                    std::this_thread::yield();

                for(int x=0; x < std::min<int>(packet.caplen, 34); x++)
                {
                    printf("%2x ", (unsigned char)buff[x]);
                }
                printf("\n");
            }
    }

    return 0;
}
 