#include"redis_client.h"
#include<string.h>
#include<iostream>
#include<stdlib.h>
#include"debug.h"
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
    if( m_predis_contx != NULL )
        redisFree(m_predis_contx);
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

void redis_client::add_svr_list(string svrlist)
{
    string2svrlist(svrlist.c_str(),m_svrlist);
}

void redis_client::add_svr(string ip,int port,int weight)
{
    struct ip_port_pair pair;
    if( ip.size()==0 || port>0xffff || port<0)
        return;
    pair.ip = ip;
    pair.port = port;
    pair.weight = weight;
    m_svrlist.push_back(pair);
}

void redis_client::add_svr(const char *ip,int port,int weight)
{
   string strip(ip);
   add_svr(strip,port,weight);
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
    struct redisContext *tmp=NULL;
    int ret = 0,online=0;
    int i;
    for(i = 0;i<m_svrlist.size();i++)
    {
        if(m_svrlist[i].port<=0 ||m_svrlist[i].port>=0xffff)
            continue;
        m_predis_contx = redisConnectWithTimeout(m_svrlist[i].ip.c_str(),m_svrlist[i].port,m_connect_timeout);
        if(m_predis_contx == NULL)
        {
            ERROR("redis_client: connect failed!");
            continue;
        }
        if(m_predis_contx->err)
        {
            ERROR("redis_client: error occured when connecting:%s",m_predis_contx->errstr);
            redisFree(m_predis_contx);
            continue;
        }
        ret = redisSetTimeout(m_predis_contx,m_op_timeout);
        if(ret<0)
        {
            if(m_predis_contx->err)
                ERROR("redis_client:set timeout failed:%s",m_predis_contx->errstr);
            else
                ERROR("redis_client:set timeout failed!");
            continue;
        }
        break;
    }
    if(NULL != m_predis_contx)
    {
        m_current_cnnt_server = m_svrlist[i];
        return 0;
    }
    else
        return -1;
}

int redis_client::use_db()
{
    char cmd[256] = {0};
    sprintf(cmd,"select %s",m_sdb_name.c_str());
    redisReply* reply = do_command(cmd);
    if(NULL == reply)
        return -1;
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

int redis_client::is_connected()
{
    if(NULL == m_predis_contx)
        return 0;
    return 1;
}

int redis_client::get_cur_svr_connted(struct ip_port_pair& pair)
{
    if( !is_connected())
        return -1;
    pair = m_current_cnnt_server;
    return 0;
}

redisReply* redis_client::do_command(const char* cmd)
{
    if( 0 == is_connected())
        return NULL;
    redisReply* reply = (redisReply*)redisCommand(m_predis_contx,cmd);
    if(NULL == reply)
    {
        if( m_predis_contx==NULL)
        {
            close();
            return NULL;
        } 
        if(REDIS_OK != m_predis_contx->err)
        {
            ERROR("redis_client: command: \"%s\" Context err:\"%s\"",cmd,m_predis_contx->errstr);
            close();
            return NULL;
        }
    }
    if( (REDIS_REPLY_ERROR == reply->type) && (NULL != reply->str))
    {
        ERROR("redis_client: command: \"%s\" Reply err: \"%s\"",cmd,reply->str);
        freeReplyObject(reply);
        reply = NULL;
    }
    return reply;
}

int redis_client::reconnect()
{
    if(0 == is_connected())
    {
        ERROR("redis_client not connected!");
        if(0 == connect())
        {
            if(0!= use_db())
            {
                ERROR("redis_client use_db failed!");
                return -1;
            }
        }
        else
        {
            ERROR("redis_client connected failed!");
            return -1;
        }
    }
    return 0;
}

