/* 
 * File:   Json.cpp
 * Author: me
 * 
 * Created on October 18, 2015, 9:57 PM
 */

#include "Json.hpp"

#include <string.h>


using namespace std;

namespace fusedrive
{

    // Gdrive_Json_Object* gdrive_json_from_string(const char* inStr);
    Json::Json(const std::string& inStr)
    : mpJsonObj(json_tokener_parse(inStr.c_str()))
    {
        // No body
    }

    // Gdrive_Json_Object* gdrive_json_new();
    Json::Json()
    : mpJsonObj(json_object_new_object())
    {
        // No body
    }

    // void gdrive_json_kill(Gdrive_Json_Object* pObj);
    Json::~Json()
    {
        // Decrement the reference count on the underlying json_object.
        json_object_put(mpJsonObj);
    }
    
    bool Json::isValid()
    {
        return (mpJsonObj != NULL);
    }

    Json Json::getNestedObject(const string& key)
    {
        return Json(getNestedInternal(key));
    }

    string Json::getString(const string& key) const
    {
        json_object* pInnerObj = getNestedInternal(key);
        if (pInnerObj == NULL || 
                !json_object_is_type(pInnerObj, json_type_string))
        {
            // Key not found, or value is not a string.
            return "";
        }

        const char* jsonStr = json_object_get_string(pInnerObj);
        return string(jsonStr ? jsonStr : "");
    }

    int64_t Json::getInt64(const string& key, bool convertTypes, bool& success) const
    {
        json_object* pInnerObj = getNestedInternal(key);
        if (pInnerObj == NULL)
        {
            // Key not found, signal failure.
            success = false;
            return 0;
        }
        if (!convertTypes && 
                !(json_object_is_type(pInnerObj, json_type_int) || 
                json_object_is_type(pInnerObj, json_type_double))
                )
        {
            // Non-numeric type, signal failure.
            success = false;
            return 0;
        }
        
        success = true;
        return json_object_get_int64(pInnerObj);
    }

    int64_t Json::getInt64(bool convertTypes, bool& success)
    {
        return getInt64("", convertTypes, success);
    }
    
    double Json::getDouble(const string& key, bool& success)
    {
        json_object* pInnerObj = getNestedInternal(key);
        if (pInnerObj == NULL)
        {
            // Key not found, signal failure.
            success = false;
            return 0.0;
        }
        if (!(json_object_is_type(pInnerObj, json_type_int) || 
                json_object_is_type(pInnerObj, json_type_double)))
        {
            // Non-numeric type, signal failure.
            success = false;
            return 0.0;
        }
        
        success = true;
        return json_object_get_double(pInnerObj);
    }

    bool Json::getBoolean(const string& key, bool& success)
    {
        json_object* pInnerObj = getNestedInternal(key);
        if (pInnerObj == NULL)
        {
            // Key not found, signal failure.
            success = false;
            return false;
        }
        if (!(json_object_is_type(pInnerObj, json_type_int) || 
                json_object_is_type(pInnerObj, json_type_double) ||
                json_object_is_type(pInnerObj, json_type_boolean)))
        {
            // Non-numeric type, signal failure.
            success = false;
            return false;
        }

        success = true;
        return json_object_get_boolean(pInnerObj);
    }

    void Json::addString(const string& key, const string& str)
    {
        // json_object_new_xxxx should give a reference count of 1, then
        // json_object_object_add should decrement the count.  Everything should
        // balance, and the caller shouldn't need to worry about reference counts
        // or ownership.
        json_object_object_add(mpJsonObj, key.c_str(), 
                json_object_new_string(str.c_str()));
    }

    void Json::addInt64(const string& key, int64_t value)
    {
        // json_object_new_xxxx should give a reference count of 1, then
        // json_object_object_add should decrement the count.  Everything should
        // balance, and the caller shouldn't need to worry about reference counts
        // or ownership.
        json_object_object_add(mpJsonObj, key.c_str(), 
                json_object_new_int64(value));
    }

    void Json::addDouble(const string& key, double value)
    {
        // json_object_new_xxxx should give a reference count of 1, then
        // json_object_object_add should decrement the count.  Everything should
        // balance, and the caller shouldn't need to worry about reference counts
        // or ownership.
        json_object_object_add(mpJsonObj, key.c_str(), 
                json_object_new_double(value));
    }

    void Json::addBoolean(const string& key, bool value)
    {
        // json_object_new_xxxx should give a reference count of 1, then
    // json_object_object_add should decrement the count.  Everything should
    // balance, and the caller shouldn't need to worry about reference counts
    // or ownership.
    json_object_object_add(mpJsonObj, key.c_str(), 
            json_object_new_boolean(value));
    }

    Json Json::addNewArray(const string& key)
    {
        // json_object_new_xxxx should give a reference count of 1, then
        // json_object_object_add should decrement the count.  Everything should
        // balance, and the caller shouldn't need to worry about reference counts
        // or ownership.
        json_object* array = json_object_new_array();
        json_object_object_add(mpJsonObj, key.c_str(), array);

        // Return the array in case the caller needs to add elements.
        return Json(array);
    }

    void Json::addExistingArray(const string& key, Json& jArray)
    {
        // Use json_object_get() before json_object_object_add() to leave the 
    // reference count unchanged.
    json_object_object_add(mpJsonObj, key.c_str(), 
            json_object_get(jArray.mpJsonObj));
    }

    string Json::toString(bool pretty)
    {
        int flags = pretty ? 
            JSON_C_TO_STRING_PRETTY : 
            JSON_C_TO_STRING_PLAIN;
    return string(json_object_to_json_string_ext(mpJsonObj, flags));
    }

    int Json::getArrayLength(const string& key, bool& success) const
    {
        json_object* pInnerObj = getNestedInternal(key);
        if (pInnerObj == NULL || !json_object_is_type(pInnerObj, json_type_array))
        {
            // Key not found or not an array, signal failure.
            success = false;
            return 0;
        }
        
        success = true;
        return json_object_array_length(pInnerObj);
    }

    int Json::getArrayLength(bool& success)
    {
        return getArrayLength("", success);
    }
    
    Json Json::arrayGet(const string& key, int index)
    {
        json_object* pInnerObj = getNestedInternal(key);
        if (pInnerObj == NULL || !json_object_is_type(pInnerObj, json_type_array))
        {
            // Key not found, or object is not an array.
            return Json((json_object*)NULL);
        }
        return Json(json_object_array_get_idx(pInnerObj, index));
    }

    Json Json::arrayGet(int index)
    {
        return arrayGet("", index);
    }
    
    int Json::arrayAppendObject(Json& newObj)
    {
        json_object_get(newObj.mpJsonObj);
        return json_object_array_add(mpJsonObj, newObj.mpJsonObj);
    }

    int Json::arrayAppendString(const string& val)
    {
        Json tmpObj = Json(json_object_new_string(val.c_str()));
        return arrayAppendObject(tmpObj);
    }

    int Json::arrayAppendBool(bool val)
    {
        Json tmpObj = Json(json_object_new_boolean(val));
        return arrayAppendObject(tmpObj);
    }

    int Json::arrayAppendDouble(double val)
    {
        Json tmpObj = Json(json_object_new_double(val));
        return arrayAppendObject(tmpObj);
    }

    int Json::arrayAppendInt64(int64_t val)
    {
        Json tmpObj = Json(json_object_new_int64(val));
        return arrayAppendObject(tmpObj);
    }

    Json::Json(json_object* pObj)
    : mpJsonObj(pObj)
    {
        // Increment the reference count so the underlying json_object
        // doesn't disappear.
        json_object_get(pObj);
    }
    
    json_object* Json::getNestedInternal(const string& key) const
    {
        return getNestedInternal(mpJsonObj, key);
    }
    
    json_object* Json::getNestedInternal(json_object* pObj, 
            const string& key) const
    {
        if (key.empty())
        {
            // No key, just return the original object.
            return mpJsonObj;
        }

        // Just use a single string guaranteed to be at least as long as the 
        // longest key (because it's the length of all the keys put together).
        //char* currentKey = malloc(strlen(key) + 1);


        size_t startIndex = 0;
        size_t endIndex = 0;
        json_object* pLastObj = mpJsonObj;
        json_object* pNextObj;

        do
        {
            // Find the '/' dividing keys or the null terminating the entire
            // set of keys.  After the for loop executes, the current key consists
            // of the range starting with (and including) startIndex, up to (but 
            // not including) endIndex.
            endIndex = key.find('/', startIndex);
            string currentKey = (endIndex == string::npos) ? 
                key.substr(startIndex) : 
                key.substr(startIndex, endIndex - startIndex);
            
            if (!json_object_object_get_ex(pLastObj, currentKey.c_str(), &pNextObj))
            {
                // If the key isn't found, return NULL (by setting pNextObj to NULL
                // before eventually returning pNextObj).
                pLastObj = pNextObj = NULL;
                break;
            }

            pLastObj = pNextObj;
            startIndex = endIndex + 1;
        } while (endIndex != string::npos && startIndex < key.length());

        return pNextObj;
    }


}

