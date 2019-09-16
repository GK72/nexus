// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LIBGS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LIBGS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LIBGS_EXPORTS
#define LIBGS_API __declspec(dllexport)
#else
#define LIBGS_API __declspec(dllimport)
#endif

// This class is exported from the dll
class LIBGS_API Clibgs {
public:
	Clibgs(void);
	// TODO: add your methods here.
};

extern "C" LIBGS_API const char* libgs_ver;

extern "C" LIBGS_API void libgs_welcome(void);
