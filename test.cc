#include"redis_client.h"
#include<iostream>
#include<unistd.h>
#define REDIS_SVR "127.0.0.1:6379:5"

int main(int argc,char ** argv)
{
    struct ip_port_pair pair;
    string svr(REDIS_SVR);
    redis_client myredis(svr);
    string dbname("1");
    myredis.set_db(dbname);
    if( myredis.connect() ){
        std::cout<<"connect failed ..."<<endl;
        return -1;
    }
    std::cout<<"after do connect "<<endl;
    if(!myredis.is_connected())
    {
        return -1;
    }
    myredis.do_command("set keyx valuex");
    while(1){
        redisReply * rep = myredis.do_command("get keyx");
        if(rep !=NULL){
            std::cout<<"rep: "<<rep->str<<endl;
            freeReplyObject(rep);
        }
        sleep(1);
    }
    myredis.close();
    return 0;
}
