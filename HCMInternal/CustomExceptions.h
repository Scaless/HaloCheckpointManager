#pragma once
#include <stacktrace>



class HCMExceptionBase : public std::exception
{
protected:
	std::string message = "error message not set";
	std::string sourceLocation;
	std::string stackTrace;

	void MandatoryLogging()
	{
		PLOG_ERROR << "Exception thrown at " << this->sourceLocation << ", I sure hope someone handles that.";
		PLOG_ERROR << "But just in case it doesn't, here's the error info: " << this->message;
		//PLOG_ERROR << this->sourceLocation << std::endl << this->message << std::endl << this->stackTrace;
	}

public:
	explicit HCMExceptionBase(const char* msg,
		const std::source_location location = std::source_location::current())
		: message(msg), sourceLocation(std::format("@ {}::{}:{}", location.file_name(), location.function_name(), location.line()).c_str())
	{
		//std::stringstream buffer;
		//buffer << boost::stacktrace::basic_stacktrace();
		//stackTrace = buffer.str();
		stackTrace = std::to_string(std::stacktrace::current());
		MandatoryLogging();
	}

	explicit HCMExceptionBase(std::string msg,
		const std::source_location location = std::source_location::current())
		: message(msg), sourceLocation(std::format("@ {}::{}:{}", location.file_name(), location.function_name(), location.line()).c_str())
	{
		//std::stringstream buffer;
		//buffer << boost::stacktrace::basic_stacktrace();
		//stackTrace = buffer.str();
		stackTrace = std::to_string(std::stacktrace::current());
		MandatoryLogging();
	}

	std::string what()
	{
		return this->message;
	}

	std::string source()
	{
		return this->sourceLocation;
	}

	std::string trace()
	{
		return this->stackTrace;
	}
	void prepend(std::string pre)
	{
		this->message = std::string(pre + this->message);
	}

	void append(std::string app)
	{
		this->message = std::string(this->message + app);
	}

};





// Classes used for exceptions that we can gracefully handle
// Thrown during HCMInternal/singleton inits if something goes horribly wrong. Error to be shown to user then HCMInternal will shutdown
class HCMInitException : public HCMExceptionBase { using HCMExceptionBase::HCMExceptionBase; };


// Runtime exceptions 
// These will always be passed to the RuntimeExceptionHandler
class HCMRuntimeException : public HCMExceptionBase 
{ 
	using HCMExceptionBase::HCMExceptionBase; 
public:
	HCMRuntimeException(HCMInitException& init) // convert from init to runtime
		: HCMExceptionBase(init.what())
	{
		this->sourceLocation = init.source();
		this->stackTrace = init.trace();
	}
};

// thrown on Serialisation/deserialisation failures
// Also passed to RuntimeExceptionHandler
class HCMSerialisationException : public HCMExceptionBase { using HCMExceptionBase::HCMExceptionBase; };
