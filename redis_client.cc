#include"redis_client.h"
#include<string.h>
#include<iostream>
#include<stdlib.h>
#include"debug.h"
#include<unistd.h>
#define REDIS_SVR "123.123.123.123:6379:5,222.222.222.222:6379:5,127.0.0.1:6379:5"
using namespace std;
int string2svrlist(const char *svrs,vector<ip_port_pair> &svr_list)
{

    struct ip_port_pair pair;
    vector<string> vect_str;
    string svr,ip,port,weight;
    int i,len=strlen(svrs);
    char * index = (char *)svrs;
    for(;len>0;index++,len--)
    {
        if( *index == ',' )
        {
            vect_str.push_back(svr);
            svr.clear();
            continue;
        }
        svr += *index;
    }
    if(svr.size()!= 0 )
        vect_str.push_back(svr);
    vector<std::string>::const_iterator it;
    for(it=vect_str.begin();it!=vect_str.end();it++)
    {
        if( (it->find(':') == string::npos ||
                it->find(':')==it->rfind(':')) )
            continue;
        ip.clear();
        ip.append(*it,0,it->find(':'));
        if(ip.size()==0)
            continue;
        pair.ip = ip;
        port = it->substr(it->find(':')+1,it->rfind(':'));
        if( port.size()==0)
            continue;
        pair.port = atoi(port.c_str());
        if(pair.port>0xffff)
            continue;
        weight = it->substr(it->rfind(':')+1);
        if(weight.size()==0)
            continue;
        pair.weight = atoi(weight.c_str());
        svr_list.push_back(pair);
        std::cout<<pair.ip<<':'<<pair.port<<':'<<pair.weight<<'\n'<<endl;
    }
    return 0;
}

redis_client::redis_client():m_sdb_name(""),m_predis_contx(NULL)
{
    m_connect_timeout.tv_sec  = 1;
    m_connect_timeout.tv_usec = 0;
    m_op_timeout.tv_sec = 1;
    m_op_timeout.tv_usec = 0;
}

redis_client::redis_client(string svrlist):m_predis_contx(NULL)
{
    string2svrlist(svrlist.c_str(),m_svrlist);
    m_connect_timeout.tv_sec  = 1;
    m_connect_timeout.tv_usec = 0;
    m_op_timeout.tv_sec = 1;
    m_op_timeout.tv_usec = 0;

}

redis_client::~redis_client()
{
    if( m_predis_contx != NULL ){
        redisFree(m_predis_contx);
        m_predis_contx=NULL;
    }
}

void redis_client::set_db(string db)
{
    m_sdb_name = db;
}

void redis_client::set_svr_list(string svrlist)
{
    m_svrlist.clear();
    string2svrlist(svrlist.c_str(),m_svrlist);

}

void redis_client::set_connect_timeout(int time)
{
    m_connect_timeout.tv_sec = time/1000000;
    m_connect_timeout.tv_usec = time%1000000;
}

void redis_client::set_op_timeout(int time)
{
    m_op_timeout.tv_sec = time/1000000;
    m_op_timeout.tv_usec = time%1000000;
}

int redis_client::connect()
{
    int ret = 0,online=0;
    int i;
    if( m_predis_contx != NULL ){
        redisFree(m_predis_contx);
        m_predis_contx=NULL;
    }
    for(i = 0;i<m_svrlist.size();i++)
    {
        if(m_svrlist[i].port<=0 ||m_svrlist[i].port>=0xffff)
            continue;
        m_predis_contx = redisConnectWithTimeout(m_svrlist[i].ip.c_str(),m_svrlist[i].port,m_connect_timeout);
        if(m_predis_contx == NULL)
        {
            XREDIS_ERR("redis_client: connect failed!");
            continue;
        }
        if(m_predis_contx->err)
        {
            XREDIS_ERR("redis_client: error occured when connecting:%s\n",m_predis_contx->errstr);
            redisFree(m_predis_contx);
            m_predis_contx=NULL;
            continue;
        }
        ret = redisSetTimeout(m_predis_contx,m_op_timeout);
        if(ret<0)
        {
            if(m_predis_contx->err)
                XREDIS_ERR("redis_client:set timeout failed:%s and free the rediscontext\n",m_predis_contx->errstr);
            else
                XREDIS_ERR("redis_client:set timeout failed! and free the rediscontext\n");
            redisFree(m_predis_contx);
            m_predis_contx=NULL;
            continue;
        }
        break;
    }
    XREDIS_ERR("m_predis_contx=%p\n",m_predis_contx);
    if(NULL != m_predis_contx)
    {
        return 0;
    }
    else
        return -1;
}

int redis_client::use_db()
{
    char cmd[32] = {0};
    sprintf(cmd,"select %s",m_sdb_name.c_str());
    XREDIS_ERR("use_db cmd: %s\n",cmd);
    redisReply* reply = (redisReply*)redisCommand(m_predis_contx,cmd);
    if(NULL == reply)
        return -1;
    if(REDIS_OK != m_predis_contx->err)
    {
        XREDIS_ERR("redis_client: command: \"%s\" Context err:\"%s\"\n",cmd,m_predis_contx->errstr);
        freeReplyObject(reply);
        reply = NULL;
        return -1;
    }
    freeReplyObject(reply);
    reply = NULL;
    return 0;
}

int redis_client::close()
{
    if(NULL != m_predis_contx)
    {
        redisFree(m_predis_contx);
        m_predis_contx = NULL;
    }
    return 0;
}


redisReply* redis_client::do_command(const char* cmd)
{
    XREDIS_ERR("in do_command now...\n");
    if( m_predis_contx==NULL)
    {
        XREDIS_ERR("the rediscontext is NULL and try to reconnect to the redis server now...\n");
        if( 0 != reconnect() ){
            XREDIS_ERR("reconnect to the redis server failed...\n");
            close();
            return NULL;
        }
    }

    redisReply* reply  = (redisReply*)redisCommand(m_predis_contx,cmd);
    if(NULL == reply)
    {
        XREDIS_ERR("the reply is NULL\n");
        if( NULL != m_predis_contx){
            XREDIS_ERR("the rediscontext is not NULL\n");
            close();
        }
        XREDIS_ERR("try to reconnect to the redis server now...\n");
        if( 0!= reconnect() ){
            XREDIS_ERR("reconnect to the redis server error and return a NULL in do_command\n ");
            close();
            return NULL;
        }
        reply = (redisReply*)redisCommand(m_predis_contx,cmd);
        if( NULL == reply ){
            XREDIS_ERR("the reply is NULL %s %d\n",__func__,__LINE__);
            return NULL;
        }
    }else{
        XREDIS_ERR("the first do redisCommnd return a reply is not NULL\n");
    }
    if(REDIS_OK != m_predis_contx->err){
        XREDIS_ERR("redis_client: command: \"%s\" Context err:\"%s\"\n",cmd,m_predis_contx->errstr);
        freeReplyObject(reply);
        reply=NULL;
        close();
        return NULL;
    }
    if( (REDIS_REPLY_ERROR == reply->type) && (NULL != reply->str)){
        XREDIS_ERR("redis_client: command: \"%s\" Reply err: \"%s\"\n",cmd,reply->str);
        freeReplyObject(reply);
        reply = NULL;
    }
    XREDIS_ERR("end do_command now...reply= %p\n",reply);
    return reply;
}

int redis_client::reconnect()
{
        if(0 == connect()){
            if(0!= use_db()){
                XREDIS_ERR("redis_client use_db failed!\n");
                return -1;
            }
        }
        else{
            XREDIS_ERR("redis_client connected failed!\n");
            return -1;
        }
    return 0;
}

#if 1
int main(int argc,char ** argv)
{
    struct ip_port_pair pair;
    string svr(REDIS_SVR);
    redis_client myredis(svr);
    string dbname("10");
    myredis.set_db(dbname);
    if( myredis.connect() ){
        std::cout<<"connect failed ..."<<endl;
        return -1;
    }
    if( 0 != myredis.use_db() ){
        printf("use_db error,and exit\n");
        return -1;
    }
    std::cout<<"after do connect "<<endl;
    redisReply *reply=NULL;
    if( NULL == (reply=myredis.do_command("set nkeyx nvaluex") )){
        std::cout<<"do_command(set keyx valuex) error"<<std::endl;
        return -1;
    }
    freeReplyObject(reply);
    reply=NULL;
    while(1){
        redisReply * rep = myredis.do_command("get nkeyx");
        printf("\nloop ...\n");
        if(rep !=NULL){
            printf("rep: %s\n",rep->str);
            freeReplyObject(rep);
        }else{
            printf("rep=NULL\n");
        }
        sleep(1);
    }
    myredis.close();
    return 0;
}

#endif
