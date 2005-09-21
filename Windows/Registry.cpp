// Windows/Registry.cpp

#include "StdAfx.h"

#include "Windows/Registry.h"

namespace NWindows {
namespace NRegistry {

#define MYASSERT(expr) // _ASSERTE(expr)

CKey::~CKey()
{ 
  Close(); 
}

HKEY CKey::Detach()
{
  HKEY hKey = _object;
  _object = NULL;
  return hKey;
}

void CKey::Attach(HKEY hKey)
{
  MYASSERT(_object == NULL);
  _object = hKey;
}

LONG CKey::Create(HKEY parentKey, LPCTSTR keyName,
    LPTSTR keyClass, DWORD options, REGSAM accessMask,
    LPSECURITY_ATTRIBUTES securityAttributes, LPDWORD disposition)
{
  MYASSERT(parentKey != NULL);
  DWORD dispositionReal;
  HKEY key = NULL;
  LONG res = RegCreateKeyEx(parentKey, keyName, 0, keyClass, 
      options, accessMask, securityAttributes, &key, &dispositionReal);
  if (disposition != NULL)
    *disposition = dispositionReal;
  if (res == ERROR_SUCCESS)
  {
    res = Close();
    _object = key;
  }
  return res;
}

LONG CKey::Open(HKEY parentKey, LPCTSTR keyName, REGSAM accessMask)
{
  MYASSERT(parentKey != NULL);
  HKEY key = NULL;
  LONG res = RegOpenKeyEx(parentKey, keyName, 0, accessMask, &key);
  if (res == ERROR_SUCCESS)
  {
    res = Close();
    MYASSERT(res == ERROR_SUCCESS);
    _object = key;
  }
  return res;
}

LONG CKey::Close()
{
  LONG res = ERROR_SUCCESS;
  if (_object != NULL)
  {
    res = RegCloseKey(_object);
    _object = NULL;
  }
  return res;
}

// win95, win98: deletes sunkey and all its subkeys
// winNT to be deleted must not have subkeys
LONG CKey::DeleteSubKey(LPCTSTR subKeyName)
{
  MYASSERT(_object != NULL);
  return RegDeleteKey(_object, subKeyName);
}

LONG CKey::RecurseDeleteKey(LPCTSTR subKeyName)
{
  CKey key;
  LONG res = key.Open(_object, subKeyName, KEY_READ | KEY_WRITE);
  if (res != ERROR_SUCCESS)
    return res;
  FILETIME fileTime;
  const UINT32 kBufferSize = MAX_PATH + 1; // 256 in ATL
  DWORD size = kBufferSize;
  TCHAR buffer[kBufferSize];
  while (RegEnumKeyEx(key._object, 0, buffer, &size, NULL, NULL, NULL,
      &fileTime) == ERROR_SUCCESS)
  {
    res = key.RecurseDeleteKey(buffer);
    if (res != ERROR_SUCCESS)
      return res;
    size = kBufferSize;
  }
  key.Close();
  return DeleteSubKey(subKeyName);
}


/////////////////////////
// Value Functions

static inline UINT32 BoolToUINT32(bool value)
  {  return (value ? 1: 0); }

static inline bool UINT32ToBool(UINT32 value)
  {  return (value != 0); }


LONG CKey::DeleteValue(LPCTSTR value)
{
  MYASSERT(_object != NULL);
  return RegDeleteValue(_object, (LPTSTR)value);
}

LONG CKey::SetValue(LPCTSTR valueName, UINT32 value)
{
  MYASSERT(_object != NULL);
  return RegSetValueEx(_object, valueName, NULL, REG_DWORD,
      (BYTE * const)&value, sizeof(UINT32));
}

LONG CKey::SetValue(LPCTSTR valueName, bool value)
{
  return SetValue(valueName, BoolToUINT32(value));
}

LONG CKey::SetValue(LPCTSTR valueName, LPCTSTR value)
{
  MYASSERT(value != NULL);
  MYASSERT(_object != NULL);
  return RegSetValueEx(_object, valueName, NULL, REG_SZ,
      (const BYTE * )value, (lstrlen(value) + 1) * sizeof(TCHAR));
}

LONG CKey::SetValue(LPCTSTR valueName, const CSysString &value)
{
  MYASSERT(value != NULL);
  MYASSERT(_object != NULL);
  return RegSetValueEx(_object, valueName, NULL, REG_SZ,
      (const BYTE *)(const TCHAR *)value, (value.Length() + 1) * sizeof(TCHAR));
}

LONG CKey::SetValue(LPCTSTR valueName, const void *value, UINT32 size)
{
  MYASSERT(value != NULL);
  MYASSERT(_object != NULL);
  return RegSetValueEx(_object, valueName, NULL, REG_BINARY,
      (const BYTE *)value, size);
}

LONG SetValue(HKEY parentKey, LPCTSTR keyName, 
    LPCTSTR valueName, LPCTSTR value)
{
  MYASSERT(value != NULL);
  CKey key;
  LONG res = key.Create(parentKey, keyName);
  if (res == ERROR_SUCCESS)
    res = key.SetValue(valueName, value);
  return res;
}

LONG CKey::SetKeyValue(LPCTSTR keyName, LPCTSTR valueName, LPCTSTR value)
{
  MYASSERT(value != NULL);
  CKey key;
  LONG res = key.Create(_object, keyName);
  if (res == ERROR_SUCCESS)
    res = key.SetValue(valueName, value);
  return res;
}

LONG CKey::QueryValue(LPCTSTR valueName, UINT32 &value)
{
  DWORD type = NULL;
  DWORD count = sizeof(DWORD);
  LONG res = RegQueryValueEx(_object, (LPTSTR)valueName, NULL, &type,
    (LPBYTE)&value, &count);
  MYASSERT((res!=ERROR_SUCCESS) || (type == REG_DWORD));
  MYASSERT((res!=ERROR_SUCCESS) || (count == sizeof(UINT32)));
  return res;
}

LONG CKey::QueryValue(LPCTSTR valueName, bool &value)
{
  UINT32 uintValue = BoolToUINT32(value);
  LONG res = QueryValue(valueName, uintValue);
  value = UINT32ToBool(uintValue);
  return res;
}

LONG CKey::QueryValue(LPCTSTR valueName, LPTSTR value, UINT32 &count)
{
  MYASSERT(count != NULL);
  DWORD type = NULL;
  LONG res = RegQueryValueEx(_object, (LPTSTR)valueName, NULL, &type,
      (LPBYTE)value, (DWORD *)&count);
  MYASSERT((res!=ERROR_SUCCESS) || (type == REG_SZ) ||
      (type == REG_MULTI_SZ) || (type == REG_EXPAND_SZ));
  return res;
}

LONG CKey::QueryValue(LPCTSTR valueName, CSysString &value)
{
  value.Empty();
  DWORD type = NULL;
  UINT32 currentSize = 0;
  LONG res = RegQueryValueEx(_object, (LPTSTR)valueName, NULL, &type,
      NULL, (DWORD *)&currentSize);
  if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA)
    return res;
  res = QueryValue(valueName, value.GetBuffer(currentSize), currentSize);
  value.ReleaseBuffer();
  return res;
}

LONG CKey::QueryValue(LPCTSTR valueName, void *value, UINT32 &count)
{
  DWORD type = NULL;
  LONG res = RegQueryValueEx(_object, (LPTSTR)valueName, NULL, &type,
    (LPBYTE)value, (DWORD *)&count);
  MYASSERT((res!=ERROR_SUCCESS) || (type == REG_BINARY));
  return res;
}


LONG CKey::QueryValue(LPCTSTR valueName, CByteBuffer &value, UINT32 &dataSize)
{
  DWORD type = NULL;
  dataSize = 0;
  LONG res = RegQueryValueEx(_object, (LPTSTR)valueName, NULL, &type,
      NULL, (DWORD *)&dataSize);
  if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA)
    return res;
  value.SetCapacity(dataSize);
  return QueryValue(valueName, (BYTE *)value, dataSize);
}

LONG CKey::EnumKeys(CSysStringVector &keyNames)
{
  keyNames.Clear();
  CSysString keyName;
  for(UINT32 index = 0; true; index++)
  {
    const UINT32 kBufferSize = MAX_PATH + 1; // 256 in ATL
    FILETIME lastWriteTime;
    UINT32 aNameSize = kBufferSize;
    LONG aResult = ::RegEnumKeyEx(_object, index, keyName.GetBuffer(kBufferSize), 
        (DWORD *)&aNameSize, NULL, NULL, NULL, &lastWriteTime);
    keyName.ReleaseBuffer();
    if(aResult == ERROR_NO_MORE_ITEMS)
      break;
    if(aResult != ERROR_SUCCESS)
      return aResult;
    keyNames.Add(keyName);
  }
  return ERROR_SUCCESS;
}


}}