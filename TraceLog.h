#pragma once

void GetTraceLog(CString& String);
void AddTraceLog(TCHAR* FormatString, va_list args);
void AddTraceLog(TCHAR* FormatString, ...);