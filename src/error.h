/*! \file error.h
 *  \brief Contains an error exception class.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <errno.h>
#include <string.h>
#include <sstream>
/*! \brief Error exception class
 *
 */
class Error
{
    int m_exitCode;                         //!<    The exit code for the process.
    std::stringstream m_sstream;            //!<    The error message.
    std::string m_string;                   //!<    Temporary storage for the error message.
public:
    //! \brief Copy constructor.
    Error(const Error &rhs): m_exitCode(-1)
    {
        m_exitCode = rhs.m_exitCode;
        m_sstream << rhs.m_sstream.rdbuf();
    }
    //! \brief Default constructor.
    Error(): m_exitCode(-1) { };
    //! \brief Construct from simple error message.
    Error(const char *s): m_exitCode(-1)
    {
        m_sstream << s;
    }
    //! \brief Construct with errno expanded to a message.
    Error(const char *s, int errNo): m_exitCode(-1)
    {
        m_sstream << s << ": " << strerror(errNo);
    }
    //! \brief Helper constructor for \a INTERNAL_ERROR and \a ASSERT macros.
    Error(const char *prefix, const char *filename, int lineNo): m_exitCode(-1)
    {
        m_sstream << prefix << filename << ":" << lineNo;
    }
    //! \brief Get error message as a c-string.
    const char *what()
    {
        m_string = m_sstream.str(); // must go through this std::string ...
        return m_string.c_str();
    }
    //! \brief Return a string stream to write an error message to.
    std::stringstream &stream() { return m_sstream; }
    //! \brief Return the exit code for the process.
    int exitCode() const { return m_exitCode; }
};

//! \brief Signal an internal error.
#define INTERNAL_ERROR throw(Error("internal error ", __FILE__, __LINE__))
//! \brief Check assertion.
#define ASSERT(x) do { if (!(x)) { throw(Error("assertion failed ", __FILE__, __LINE__)); } } while(0)

