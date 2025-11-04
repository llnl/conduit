// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.
#ifndef CONDUIT_YYJSON_INTERFACE_H
#define CONDUIT_YYJSON_INTERFACE_H

#include <string>
#include <sstream>
#include <conduit_yyjson.h>

using namespace conduit_yyjson;
using yyjson_error_code_and_msg = std::pair<yyjson_read_code, const char*>;
std::string GetParseError_En(yyjson_error_code_and_msg errorCodeAndMessage)
{
    std::stringstream message;
    message << "YYJSON error code " << errorCodeAndMessage.first;
    std::string errorMessage = "Unknown error code";
    switch(errorCodeAndMessage.first)
    {
        case YYJSON_READ_SUCCESS:
            errorMessage = "Success, no error.";
            break;
        case YYJSON_READ_ERROR_INVALID_PARAMETER:
            errorMessage = "Invalid parameter, such as NULL input string or 0 input length.";
            break;
        case YYJSON_READ_ERROR_MEMORY_ALLOCATION:
            errorMessage = "Memory allocation failure occurs.";
            break;
        case YYJSON_READ_ERROR_EMPTY_CONTENT:
            errorMessage = "Input JSON string is empty.";
            break;
        case YYJSON_READ_ERROR_UNEXPECTED_CONTENT:
            errorMessage = "Unexpected content after document, such as `[123]abc`.";
            break;
        case YYJSON_READ_ERROR_UNEXPECTED_END:
            errorMessage = "Unexpected ending, such as `[123`.";
            break;
        case YYJSON_READ_ERROR_UNEXPECTED_CHARACTER:
            errorMessage = "Unexpected character inside the document, such as `[abc]`.";
            break;
        case YYJSON_READ_ERROR_JSON_STRUCTURE:
            errorMessage = "Invalid JSON structure, such as `[1,]`.";
            break;
        case YYJSON_READ_ERROR_INVALID_COMMENT:
            errorMessage = "Invalid comment, such as unclosed multi-line comment.";
            break;
        case YYJSON_READ_ERROR_INVALID_NUMBER:
            errorMessage = "Invalid number, such as `123.e12`, `000`.";
            break;
        case YYJSON_READ_ERROR_INVALID_STRING:
            errorMessage = "Invalid string, such as invalid escaped character inside a string.";
            break;
        case YYJSON_READ_ERROR_LITERAL:
            errorMessage = "Invalid JSON literal, such as `truu`.";
            break;
        case YYJSON_READ_ERROR_FILE_OPEN:
            errorMessage = "Failed to open a file.";
            break;
        case YYJSON_READ_ERROR_FILE_READ:
            errorMessage = "Failed to read a file.";
            break;
        default:
            break;
    }

    message << " (" << errorMessage << "): ";
    message << errorCodeAndMessage.second;
    return message.str();
}

namespace conduit_yyjson
{
using ParseFlag = int;
static const ParseFlag kParseNoFlags = 0;
using SizeType = size_t;

/*
 * Thin wrapper to adapt the yyjson to the rapidjson API.
 */
class Value
{
public:
    Value(yyjson_val* value) : value(value)
    {
    }

    bool IsNumber() const
    {
        return yyjson_is_num(value);
    }

    bool IsUint64() const
    {
        return yyjson_is_uint(value);
    }
    bool IsInt64() const
    {
        return yyjson_is_sint(value);
    }
    bool IsUint() const
    {
        return yyjson_is_uint(value);
    }
    bool IsInt() const
    {
        return yyjson_is_int(value);
    }
    bool IsDouble() const
    {
        return yyjson_is_real(value);
    }
    bool IsString() const
    {
        return yyjson_is_str(value);
    }
    bool IsObject() const
    {
        return yyjson_is_obj(value);
    }
    bool IsArray() const
    {
        return yyjson_is_arr(value);
    }
    bool IsBool() const
    {
        return yyjson_is_bool(value);
    }
    bool IsNull() const
    {
        return yyjson_is_null(value);
    }
    bool IsTrue() const
    {
        return yyjson_is_true(value);
    }
    const char * GetString() const
    {
        return yyjson_get_str(value);
    }
    SizeType Size() const
    {
        return yyjson_get_len(value);
    }
    int64_t GetInt64() const
    {
        return yyjson_get_sint(value);
    }
    uint64_t GetUint64() const
    {
        return yyjson_get_uint(value);
    }
    double GetDouble() const
    {
        return yyjson_get_real(value);
    }
    bool GetBool() const
    {
        return yyjson_get_bool(value);
    }
    int GetInt() const
    {
        return yyjson_get_int(value);
    }
    bool HasMember(const char* name) const
    {
        return yyjson_obj_get(value, name) != nullptr;
    }
    Value operator[](SizeType index) const
    {
        return Value(yyjson_arr_get(value, index));
    }
    Value operator[](const char* name) const
    {
        return Value(yyjson_obj_get(value, name));
    }

    class ConstMemberIterator
    {
    public:
        ConstMemberIterator(const yyjson_obj_iter& iter) :
            iterator(iter),
            content{nullptr, nullptr}
        {
            Next();
        }
        ConstMemberIterator() :
            content{nullptr, nullptr}
        {
            iterator.obj = nullptr;
        }
        ConstMemberIterator& operator++()
        {
            Next();
            return *this;
        }
        bool operator!=(const ConstMemberIterator& other)
        {
            return other.iterator.obj != iterator.obj;
        }

        class Name
        {
        public:
            Name(yyjson_val* value) : value(value){}
            std::string GetString() const
            {
                return yyjson_get_str(value);
            }
            bool IsValid() const
            {
                return value != nullptr;
            }
        private:
            yyjson_val* value;
        };

        // Trick to implement the -> operator to fake the iterator.
        struct Content
        {
            Name name;
            yyjson_val* value;
        };
        const Content* operator->() const
        {
            return &content;
        }

    private:
        void Next()
        {
            auto key = yyjson_obj_iter_next(&iterator);
            if(key != nullptr)
            {
                content.value = yyjson_obj_iter_get_val(key);
                content.name = Name(key);
            }
            else
            {
                iterator.obj = nullptr;
            }
        }
        yyjson_obj_iter iterator;
        Content content;
    };

    ConstMemberIterator MemberBegin() const
    {
        auto iterator = yyjson_obj_iter_with(value);
        return ConstMemberIterator(iterator);
    }
    ConstMemberIterator MemberEnd() const
    {
        return ConstMemberIterator();
    }

protected:
    void SetValue(yyjson_val* newValue)
    {
        value = newValue;
    }
private:
    yyjson_val* value;
};

/*
 * Thin wrapper to adapt the yyjson_doc to rapidjson::Document API
 */
class Document : public Value
{
public:
    Document() : Value(nullptr), doc(nullptr), hasParseError(false)
    {
    }
    ~Document()
    {
        yyjson_doc_free(doc);
    }

    template<ParseFlag flag>
    Document& Parse(const char* text)
    {
        yyjson_doc_free(doc);
        doc = yyjson_read_opts(const_cast<char*>(text), strlen(text), flag, nullptr, &errorInformation);
        if(doc)
        {
            auto *root = yyjson_doc_get_root(doc);
            this->SetValue(root);
        }
        else
        {
            hasParseError = true;
        }
        return *this;
    }

    bool HasParseError() const
    {
        return hasParseError;
    }
    size_t GetErrorOffset() const
    {
        return errorInformation.pos;
    }
    std::pair<yyjson_read_code, const char*> GetParseError() const
    {
        return std::make_pair(errorInformation.code, errorInformation.msg);
    }


private:
    yyjson_doc* doc;
    bool hasParseError;
    yyjson_read_err errorInformation;
};

}

#endif
