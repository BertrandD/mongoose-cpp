#include "Sessions.h"
#include "Utils.h"


namespace Mongoose
{
    Sessions::Sessions(const std::string &key, Controller *controller, Server *server)
        :
          AbstractRequestPreprocessor(controller, server),
          mGcDivisor(100),
          mGcCounter(0),
          mSessions(),
          mKey(key)
    {
    }

    Sessions::~Sessions()
    {
        std::map<std::string, Session *>::iterator it;
        for (it=mSessions.begin(); it!=mSessions.end(); it++)
        {
            delete (*it).second;
        }
    }

    std::string Sessions::getId(std::weak_ptr<Request> request, std::weak_ptr<Response> response)
    {
        auto req = request.lock();
        auto res = response.lock();
        assert(req);
        assert(res);

        if (req->hasCookie(mKey)) {
            return req->getCookie(mKey);
        } else {

            std::string newCookie = Utils::randomAlphanumericString(30);
            res->setCookie(mKey, newCookie);
            return newCookie;
        }
    }

    Session* Sessions::get(std::weak_ptr<Request> request, std::weak_ptr<Response> response)
    { 
        std::string id = getId(request, response);
        Session *session = NULL;
        
        if (mSessions.find(id) != mSessions.end()) {
            session = mSessions[id];
        } else {
            session = new Session();
            mSessions[id] = session;
        }

        return session;
    }

    void Sessions::garbageCollect(int oldAge)
    {
        std::vector<std::string> deleteList;
        std::map<std::string, Session*>::iterator it;
        std::vector<std::string>::iterator vit;

        for (it=mSessions.begin(); it!=mSessions.end(); it++) {
            std::string name = (*it).first;
            Session *session = (*it).second;

            if (session->getAge() > oldAge) {
                delete session;
                deleteList.push_back(name);
            }
        }

        for (vit=deleteList.begin(); vit!=deleteList.end(); vit++) {
            mSessions.erase(*vit);
        }
    }

    bool Sessions::preProcess(std::weak_ptr<Request> request, std::weak_ptr<Response> response)
    {
        mGcCounter++;

        if (mGcCounter > mGcDivisor)
        {
            mGcCounter = 0;
            garbageCollect();
        }

        get(request, response)->ping();
        return true;
    }
}