#include"redis_client.h"
#include<iostream>

#define REDIS_SVR "127.0.0.1:6379:5,182.92.222.246:6379:4,22.22.22.22:637:889"

int main(int argc,char ** argv)
{
    struct ip_port_pair pair;
    string svr(REDIS_SVR);
    redis_client myredis(svr);
    string dbname("xiao");
    myredis.set_db(dbname);
    myredis.connect();
    std::cout<<"after do connect "<<endl;
    if(!myredis.is_connected())
    {
        return -1;
    }
    myredis.do_command("set keyx valuex");
    redisReply * rep = myredis.do_command("get keyx");
    std::cout<<"rep: "<<rep->str<<endl;
    if( 0==myredis.get_cur_svr_connted(pair) )
        std::cout<<"current server:\nip="<<pair.ip<<"\nport="<<pair.port<<"\nweight="<<pair.weight<<"\n"<<endl;
    freeReplyObject(rep);
    myredis.close();
    return 0;
}
