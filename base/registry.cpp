
#include "registry.h"

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib") // SHDeleteKey

#include "logging.h"

namespace base
{

    RegistryValueIterator::RegistryValueIterator(HKEY root_key,
        const wchar_t* folder_key)
    {
        LONG result = RegOpenKeyEx(root_key, folder_key, 0, KEY_READ, &key_);
        if(result != ERROR_SUCCESS)
        {
            key_ = NULL;
        }
        else
        {
            DWORD count = 0;
            result = ::RegQueryInfoKey(key_, NULL, 0, NULL, NULL, NULL, NULL,
                &count, NULL, NULL, NULL, NULL);

            if(result != ERROR_SUCCESS)
            {
                ::RegCloseKey(key_);
                key_ = NULL;
            }
            else
            {
                index_ = count - 1;
            }
        }

        Read();
    }

    RegistryValueIterator::~RegistryValueIterator()
    {
        if(key_)
        {
            ::RegCloseKey(key_);
        }
    }

    bool RegistryValueIterator::Valid() const
    {
        return key_!=NULL && index_>=0;
    }

    void RegistryValueIterator::operator++()
    {
        --index_;
        Read();
    }

    bool RegistryValueIterator::Read()
    {
        if(Valid())
        {
            DWORD ncount = arraysize(name_);
            value_size_ = sizeof(value_);
            LRESULT r = ::RegEnumValue(key_, index_, name_, &ncount, NULL,
                &type_, reinterpret_cast<BYTE*>(value_), &value_size_);
            if(ERROR_SUCCESS == r)
            {
                return true;
            }
        }

        name_[0] = '\0';
        value_[0] = '\0';
        value_size_ = 0;
        return false;
    }

    DWORD RegistryValueIterator::ValueCount() const
    {
        DWORD count = 0;
        HRESULT result = ::RegQueryInfoKey(key_, NULL, 0, NULL, NULL, NULL,
            NULL, &count, NULL, NULL, NULL, NULL);

        if(result != ERROR_SUCCESS)
        {
            return 0;
        }

        return count;
    }

    RegistryKeyIterator::RegistryKeyIterator(HKEY root_key,
        const wchar_t* folder_key)
    {
        LONG result = RegOpenKeyEx(root_key, folder_key, 0, KEY_READ, &key_);
        if(result != ERROR_SUCCESS)
        {
            key_ = NULL;
        }
        else
        {
            DWORD count = 0;
            HRESULT result = ::RegQueryInfoKey(key_, NULL, 0, NULL, &count,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL);

            if(result != ERROR_SUCCESS)
            {
                ::RegCloseKey(key_);
                key_ = NULL;
            }
            else
            {
                index_ = count - 1;
            }
        }

        Read();
    }

    RegistryKeyIterator::~RegistryKeyIterator()
    {
        if(key_)
        {
            ::RegCloseKey(key_);
        }
    }

    bool RegistryKeyIterator::Valid() const
    {
        return key_!=NULL && index_>=0;
    }

    void RegistryKeyIterator::operator++()
    {
        --index_;
        Read();
    }

    bool RegistryKeyIterator::Read()
    {
        if(Valid())
        {
            DWORD ncount = arraysize(name_);
            FILETIME written;
            LRESULT r = ::RegEnumKeyEx(key_, index_, name_, &ncount,
                NULL, NULL, NULL, &written);
            if(ERROR_SUCCESS == r)
            {
                return true;
            }
        }

        name_[0] = '\0';
        return false;
    }

    DWORD RegistryKeyIterator::SubkeyCount() const
    {
        DWORD count = 0;
        HRESULT result = ::RegQueryInfoKey(key_, NULL, 0, NULL, &count,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        if(result != ERROR_SUCCESS)
        {
            return 0;
        }

        return count;
    }

    RegKey::RegKey() : key_(NULL), watch_event_(0) {}

    RegKey::RegKey(HKEY rootkey, const wchar_t* subkey, REGSAM access)
        : key_(NULL), watch_event_(0)
    {
        if(rootkey)
        {
            if(access & (KEY_SET_VALUE|KEY_CREATE_SUB_KEY|KEY_CREATE_LINK))
            {
                Create(rootkey, subkey, access);
            }
            else
            {
                Open(rootkey, subkey, access);
            }
        }
        else
        {
            DCHECK(!subkey);
        }
    }

    RegKey::~RegKey()
    {
        Close();
    }

    void RegKey::Close()
    {
        StopWatching();
        if(key_)
        {
            ::RegCloseKey(key_);
            key_ = NULL;
        }
    }

    bool RegKey::Create(HKEY rootkey, const wchar_t* subkey, REGSAM access)
    {
        DWORD disposition_value;
        return CreateWithDisposition(rootkey, subkey, &disposition_value, access);
    }

    bool RegKey::CreateWithDisposition(HKEY rootkey, const wchar_t* subkey,
        DWORD* disposition, REGSAM access)
    {
        DCHECK(rootkey && subkey && access && disposition);
        Close();

        LONG result = RegCreateKeyEx(rootkey,
            subkey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            access,
            NULL,
            &key_,
            disposition);
        if(result != ERROR_SUCCESS)
        {
            key_ = NULL;
            return false;
        }

        return true;
    }

    bool RegKey::Open(HKEY rootkey, const wchar_t* subkey, REGSAM access)
    {
        DCHECK(rootkey && subkey && access);
        Close();

        LONG result = RegOpenKeyEx(rootkey, subkey, 0, access, &key_);
        if(result != ERROR_SUCCESS)
        {
            key_ = NULL;
            return false;
        }
        return true;
    }

    bool RegKey::CreateKey(const wchar_t* name, REGSAM access)
    {
        DCHECK(name && access);

        HKEY subkey = NULL;
        LONG result = RegCreateKeyEx(key_, name, 0, NULL,
            REG_OPTION_NON_VOLATILE, access, NULL, &subkey, NULL);
        Close();

        key_ = subkey;
        return (result == ERROR_SUCCESS);
    }

    bool RegKey::OpenKey(const wchar_t* name, REGSAM access)
    {
        DCHECK(name && access);

        HKEY subkey = NULL;
        LONG result = RegOpenKeyEx(key_, name, 0, access, &subkey);

        Close();

        key_ = subkey;
        return (result == ERROR_SUCCESS);
    }

    DWORD RegKey::ValueCount()
    {
        DWORD count = 0;
        HRESULT result = RegQueryInfoKey(key_, NULL, 0, NULL, NULL,
            NULL, NULL, &count, NULL, NULL, NULL, NULL);
        return (result!=ERROR_SUCCESS) ? 0 : count;
    }

    bool RegKey::ReadName(int index, std::wstring* name)
    {
        wchar_t buf[256];
        DWORD bufsize = arraysize(buf);
        LRESULT r = ::RegEnumValue(key_, index, buf, &bufsize, NULL,
            NULL, NULL, NULL);
        if(r != ERROR_SUCCESS)
        {
            return false;
        }
        if(name)
        {
            *name = buf;
        }
        return true;
    }

    bool RegKey::ValueExists(const wchar_t* name)
    {
        if(!key_)
        {
            return false;
        }
        HRESULT result = RegQueryValueEx(key_, name, 0, NULL, NULL, NULL);
        return (result == ERROR_SUCCESS);
    }

    bool RegKey::ReadValue(const wchar_t* name, void* data,
        DWORD* dsize, DWORD* dtype)
    {
        if(!key_)
        {
            return false;
        }
        HRESULT result = RegQueryValueEx(key_, name, 0, dtype,
            reinterpret_cast<LPBYTE>(data), dsize);
        return (result == ERROR_SUCCESS);
    }

    bool RegKey::ReadValue(const wchar_t* name, std::wstring* value)
    {
        DCHECK(value);
        const size_t kMaxStringLength = 1024; // 展开后的长度.
        // 如果1024太小, 使用其它ReadValue方式.
        wchar_t raw_value[kMaxStringLength];
        DWORD type = REG_SZ, size = sizeof(raw_value);
        if(ReadValue(name, raw_value, &size, &type))
        {
            if(type == REG_SZ)
            {
                *value = raw_value;
            }
            else if(type == REG_EXPAND_SZ)
            {
                wchar_t expanded[kMaxStringLength];
                size = ExpandEnvironmentStrings(raw_value, expanded, kMaxStringLength);
                // 成功: 返回拷贝的字符串长度.
                // 失败: 如果缓冲区太小返回需要的长度.
                // 失败: 其它返回0.
                if(size==0 || size>kMaxStringLength)
                {
                    return false;
                }
                *value = expanded;
            }
            else
            {
                // 不是字符串.
                return false;
            }
            return true;
        }

        return false;
    }

    bool RegKey::ReadValueDW(const wchar_t* name, DWORD* value)
    {
        DCHECK(value);
        DWORD type = REG_DWORD;
        DWORD size = sizeof(DWORD);
        DWORD result = 0;
        if(ReadValue(name, &result, &size, &type) &&
            (type==REG_DWORD || type==REG_BINARY) &&
            size==sizeof(DWORD))
        {
            *value = result;
            return true;
        }

        return false;
    }

    bool RegKey::WriteValue(const wchar_t* name, const void * data,
        DWORD dsize, DWORD dtype)
    {
        DCHECK(data);

        if(!key_)
        {
            return false;
        }

        HRESULT result = RegSetValueEx(
            key_,
            name,
            0,
            dtype,
            reinterpret_cast<LPBYTE>(const_cast<void*>(data)),
            dsize);
        return (result == ERROR_SUCCESS);
    }

    bool RegKey::WriteValue(const wchar_t * name, const wchar_t* value)
    {
        return WriteValue(name, value,
            static_cast<DWORD>(sizeof(*value)*(wcslen(value)+1)), REG_SZ);
    }

    bool RegKey::WriteValue(const wchar_t* name, DWORD value)
    {
        return WriteValue(name, &value,
            static_cast<DWORD>(sizeof(value)), REG_DWORD);
    }

    bool RegKey::DeleteKey(const wchar_t* name)
    {
        return (!key_) ? false : (ERROR_SUCCESS==SHDeleteKey(key_, name));
    }

    bool RegKey::DeleteValue(const wchar_t* value_name)
    {
        DCHECK(value_name);
        HRESULT result = RegDeleteValue(key_, value_name);
        return (result == ERROR_SUCCESS);
    }

    bool RegKey::StartWatching()
    {
        if(!watch_event_)
        {
            watch_event_ = CreateEvent(NULL, TRUE, FALSE, NULL);
        }

        DWORD filter = REG_NOTIFY_CHANGE_NAME |
            REG_NOTIFY_CHANGE_ATTRIBUTES |
            REG_NOTIFY_CHANGE_LAST_SET |
            REG_NOTIFY_CHANGE_SECURITY;

        // Watch the registry key for a change of value.
        HRESULT result = RegNotifyChangeKeyValue(key_, TRUE, filter,
            watch_event_, TRUE);
        if(SUCCEEDED(result))
        {
            return true;
        }
        else
        {
            CloseHandle(watch_event_);
            watch_event_ = 0;
            return false;
        }
    }

    bool RegKey::StopWatching()
    {
        if(watch_event_)
        {
            CloseHandle(watch_event_);
            watch_event_ = 0;
            return true;
        }
        return false;
    }

    bool RegKey::HasChanged()
    {
        if(watch_event_)
        {
            if(WaitForSingleObject(watch_event_, 0) == WAIT_OBJECT_0)
            {
                StartWatching();
                return true;
            }
        }
        return false;
    }

} //namespace base