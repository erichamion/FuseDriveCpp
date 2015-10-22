/* 
 * File:   Json.hpp
 * Author: me
 *
 * Created on October 18, 2015, 9:57 PM
 */

#ifndef JSON_HPP
#define	JSON_HPP

#include <json-c/json.h>
#include <string>


namespace fusedrive
{

    class Json {
    public:
        // Gdrive_Json_Object* gdrive_json_from_string(const char* inStr);
        Json(const std::string& inStr);
        
        // Gdrive_Json_Object* gdrive_json_new();
        Json();
        
        Json(const Json& orig);
        
        // void gdrive_json_kill(Gdrive_Json_Object* pObj);
        virtual ~Json();
        
        bool isValid();
        
        Json getNestedObject(const std::string& key="");

        std::string getString(const std::string& key="");

        int64_t getInt64(const std::string& key, bool convertTypes, 
            bool& success);
        
        int64_t getInt64(bool convertTypes, bool& success);

        double getDouble(const std::string& key, bool& success);

        bool getBoolean(const std::string& key, bool& success);

        void addString(const std::string& key, 
            const std::string& str);

        void addInt64(const std::string& key, int64_t value);

        void addDouble(const std::string& key, double value);

        void addBoolean(const std::string& key, bool value);

        Json addNewArray(const std::string& key);

        void addExistingArray(const std::string& key, Json& jArray);

        std::string toString(bool pretty);

        int getArrayLength(const std::string& key, bool& success);
        
        int getArrayLength(bool& success);

        Json arrayGet(const std::string& key, int index);
        
        Json arrayGet(int index);

        int arrayAppendObject(Json& newObj);

        int arrayAppendString(const std::string& val);

        int arrayAppendBool(bool val);

        int arrayAppendDouble(double val);

        int arrayAppendInt64(int64_t val);

        
    private:
        json_object* mpJsonObj;
        
        Json(json_object* pObj);
        
        json_object* getNestedInternal(const std::string& key);
        
        json_object* getNestedInternal(json_object* pObj, 
            const std::string& key);
        
    };

}

#endif	/* JSON_HPP */

