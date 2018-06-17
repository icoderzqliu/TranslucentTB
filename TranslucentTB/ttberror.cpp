#include "ttberror.hpp"
#include <comdef.h>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <winerror.h>
#include <WinUser.h>

#include "autofree.hpp"
#include "common.hpp"
#include "ttblog.hpp"
#include "util.hpp"

bool Error::Handle(const HRESULT &error, const Level &level, const wchar_t *const message, const wchar_t *const file, const int &line, const char *const function)
{
	if (FAILED(error))
	{
		const std::wstring message_str(message);
		const std::wstring error_message = ExceptionFromHRESULT(error);
		std::wostringstream boxbuffer;
		if (level != Level::Log && level != Level::Debug)
		{
			boxbuffer << message_str << L"\n\n";

			if (level == Level::Fatal)
			{
				boxbuffer << L"Program will exit.\n\n";
			}

			boxbuffer << error_message;
		}

		const size_t functionLength = std::char_traits<char>::length(function);
		std::wstring functionW;
		functionW.resize(functionLength);
		if (!MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, function, functionLength, functionW.data(), functionLength))
		{
			functionW = L"[failed to convert function name to UTF-16]";
		}

		std::wostringstream err;
		err << message_str << L' ' << error_message <<
			L" (" << file << L':' << line << L" at function " << functionW << L')';

		switch (level)
		{
		case Level::Debug:
			err << L'\n';
			OutputDebugString(err.str().c_str());
			break;
		case Level::Log:
			Log::OutputMessage(err.str());
			break;
		case Level::Error:
			Log::OutputMessage(err.str());
			MessageBox(NULL, boxbuffer.str().c_str(), NAME L" - Error", MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
			break;
		case Level::Fatal:
			Log::OutputMessage(err.str());
			MessageBox(NULL, boxbuffer.str().c_str(), NAME L" - Fatal error", MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
			std::terminate();
			break;
		default:
			throw std::invalid_argument("level was not one of known values");
			break;
		}

		return false;
	}
	else
	{
		return true;
	}
}

std::wstring Error::ExceptionFromHRESULT(const HRESULT &result)
{
	AutoFree::SilentLocal<wchar_t> error;
	const DWORD count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr, result, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), reinterpret_cast<wchar_t *>(&error), 0, nullptr);
	std::wostringstream stream;
	stream << L"Exception from HRESULT: " << (count ? Util::Trim(error.data()) : L"[Failed to get error message for HRESULT]") <<
		L" (0x" << std::setw(sizeof(HRESULT) * 2) << std::setfill(L'0') << std::hex << result << L')';
	return stream.str();
}