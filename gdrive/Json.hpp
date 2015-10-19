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
        
        bool gdrive_json_is_valid();
        
        Json gdrive_json_get_nested_object(const std::string& key="");

        std::string gdrive_json_get_string(const std::string& key="");

        int64_t gdrive_json_get_int64(const std::string& key, 
            bool convertTypes, bool& success);
        
        int64_t gdrive_json_get_int64(bool convertTypes, bool& success);

        double gdrive_json_get_double(const std::string& key, bool& success);

        bool gdrive_json_get_boolean(const std::string& key, bool& success);

        void gdrive_json_add_string(const std::string& key, 
            const std::string& str);

        void gdrive_json_add_int64(const std::string& key, int64_t value);

        void gdrive_json_add_double(const std::string& key, double value);

        void gdrive_json_add_boolean(const std::string& key, bool value);

        Json gdrive_json_add_new_array(const std::string& key);

        void gdrive_json_add_existing_array(const std::string& key, 
            Json& jArray);

        std::string gdrive_json_to_string(bool pretty);

        int gdrive_json_array_length(const std::string& key, bool& success);
        
        int gdrive_json_array_length(bool& success);

        Json gdrive_json_array_get(const std::string& key, int index);
        
        Json gdrive_json_array_get(int index);

        int gdrive_json_array_append_object(Json& newObj);

        int gdrive_json_array_append_string(const std::string& val);

        int gdrive_json_array_append_bool(bool val);

        int gdrive_json_array_append_double(double val);

        int gdrive_json_array_append_int64(int64_t val);

        
    private:
        json_object* pJsonObj;
        
        Json(json_object* pObj);
        
        json_object* gdrive_json_get_nested_internal(const std::string& key);
        
        json_object* gdrive_json_get_nested_internal(json_object* pObj, 
            const std::string& key);
        
    };

}

#endif	/* JSON_HPP */

