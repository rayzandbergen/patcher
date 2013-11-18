/*! \file error.h
 *  \brief Contains an error exception class.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <errno.h>
#include <string.h>
#include <sstream>
/*! \brief Error exception class
 */
class Error
{
    int m_exitCode;                         //!<    The intended exit code for the process.
    std::stringstream m_sstream;            //!<    Stream access to the error message.
    std::string m_tempString;               //!<    Temporary string buffer.
public:
    //! \brief Copy constructor.
    Error(const Error &rhs)
    {
        m_exitCode = rhs.m_exitCode;
        m_sstream << rhs.m_sstream.rdbuf();
    }
    //! \brief Assignment operator.
    Error &operator=(const Error &rhs)
    {
        m_exitCode = rhs.m_exitCode;
        m_sstream << rhs.m_sstream.rdbuf();
        // m_tempString not copied
        return *this;
    }
    //! \brief Default constructor.
    Error(): m_exitCode(-1) { };
    //! \brief Construct from simple error message.
    Error(const char *s): m_exitCode(-1)
    {
        m_sstream << s;
    }
    //! \brief Construct with errno expanded to a message, like perror(3).
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
        m_tempString = m_sstream.str();
        return m_tempString.c_str();
    }
    //! \brief Return a string stream to write an error message to.
    std::stringstream &stream() { return m_sstream; }
    //! \brief Get the intended exit code for the process.
    int exitCode() const { return m_exitCode; }
    //! \brief Set the intended exit code for the process.
    int &exitCode() { return m_exitCode; }
};

//! \brief Signal an internal error.
#define INTERNAL_ERROR throw(Error("internal error ", __FILE__, __LINE__))
//! \brief Check assertion.
#define ASSERT(x) do { if (!(x)) { throw(Error("assertion failed ", __FILE__, __LINE__)); } } while(0)

