/**
 * @file   utils.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  string utils (from qx)
 */

#ifndef QL_UTILS_H
#define QL_UTILS_H

#include "str.h"

#include <limits>
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>
#include <utility>
#include <vector>


#define println(x) std::cout << "[OPENQL] "<< x << std::endl

size_t MAX_CYCLE = std::numeric_limits<int>::max();

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace ql
{
    /**
     * utils
     */
    namespace utils
    {
        void make_output_dir(std::string dir)
        {
            #if defined(_WIN32)
            _mkdir(dir.c_str());
            #else
            mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            #endif
        }

        /**
         * @param str
         *    string to be processed
         * @param seq
         *    string to be replaced
         * @param rep
         *    string used to replace seq
         * @brief
         *    replace recursively seq by rep in str
         */
        void replace_all(std::string &str, std::string seq, std::string rep)
        {
            str::replace_all(str,seq,rep);
        }

        /**
         * string starts with " and end with "
         * return the content of the string between the commas
         */
        bool format_string(std::string& s)
        {
            replace_all(s,"\\n","\n");
            size_t pf = s.find("\"");
            if (pf == std::string::npos)
                return false;
            size_t ps = s.find_last_of("\"");
            if (ps == std::string::npos)
                return false;
            if (ps == pf)
                return false;
            s = s.substr(pf+1,ps-pf-1);
            return true;
        }

        /**
        * write content to the file <file_name>
        */
        void write_file(std::string file_name, const std::string& content)
        {
            std::ofstream file;
            file.open(file_name);
            if ( file.fail() )
            {
                std::cout << "[x] error opening file '" << file_name << "' !" << std::endl
                          << "         make sure the output directory exists for '" << file_name << "'" << std::endl;
                return;
            }

            file << content;
            file.close();
        }

        #include <sys/stat.h>
        bool exists_file(const std::string& file_name)
        {
            struct stat buffer;   
            return (stat (file_name.c_str(), &buffer) == 0); 
        }

        template <typename T>
        std::string to_string(T arg)
        {
            std::stringstream ss;
            ss << arg;
            return ss.str ();
        }

        template<class T>
        std::string to_string(std::vector<T> v, std::string vector_prefix = "",
                              std::string elem_sep = ", ")
        {
            std::ostringstream ss;
            ss << vector_prefix << " [";
            size_t sz = v.size();
            if(sz > 0)
            {
                size_t i;
                for (i=0; i<sz-1; ++i)
                    ss << v[i] << elem_sep;
                ss << v[i];
            }

            ss << "]";
            return ss.str();
        }


        /**
         * print vector
         */
        template<typename T>
        void print_vector(std::vector<T> v, std::string prefix="", std::string separator=" | ")
        {
            std::cout << to_string(v, prefix, separator) << std::endl;
        }

        template <typename T>
        int sign_of(T val)
        {
            return (T(0) < val) - (val < T(0));
        }


        bool string_has(const std::string & str, const std::string & token)
        {
            return ( str.find(token) != std::string::npos);
        }

        // Helper function to sort the vector of pairs.
        // Pairs are sorted by first element of pairs and then by second element
        bool sort_pair_helper(const std::pair<size_t,size_t> &a, const std::pair<size_t,size_t> &b)
        {
            if(a.first < b.first)
                return true;
            else if (a.first == b.first)
                return (a.second < b.second);
            else
                return false;
        }

        namespace logger
        {
            enum log_level_t
            {
                LOG_NOTHING,
                LOG_CRITICAL,
                LOG_ERROR,
                LOG_WARNING,
                LOG_INFO,
                LOG_DEBUG
            };
            log_level_t LOG_LEVEL;

            void set_log_level(std::string level)
            {
                if(level == "LOG_NOTHING")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_NOTHING;
                else if(level == "LOG_CRITICAL")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_CRITICAL;
                else if(level == "LOG_ERROR")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_ERROR;
                else if(level == "LOG_WARNING")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_WARNING;
                else if(level == "LOG_INFO")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_INFO;
                else if(level == "LOG_DEBUG")
                    ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_DEBUG;
                else
                    std::cerr << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" Error: Unknown log level" << std::endl;
            }

        } // logger namespace

    } // utils namespace

} // ql namespace

#define EOUT(content) \
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_ERROR ) \
        std::cerr << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" Error: "<< content << std::endl

#define WOUT(content) \
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_WARNING ) \
        std::cerr << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" Warning: "<< content << std::endl

#define IOUT(content) \
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_INFO ) \
        std::cout << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" Info: "<< content << std::endl

#define DOUT(content) \
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG ) \
        std::cout << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" "<< content << std::endl

#define COUT(content) \
        std::cout << "[OPENQL] " << __FILE__ <<":"<< __LINE__ <<" "<< content << std::endl

// helper macro: stringstream to string
// based on https://stackoverflow.com/questions/21924156/how-to-initialize-a-stdstringstream
#define SS2S(values) std::string(static_cast<std::ostringstream&&>(std::ostringstream() << values).str())

#define FATAL(content) \
        {   EOUT(content); \
            throw ql::exception(SS2S("Error : " << content), false); \
        }

// get the number of elements in an array
#define ELEM_CNT(x) (sizeof(x)/sizeof(x[0]))

// check existence of JSON key within node, see PR #194
#define JSON_EXISTS(node, key)  (node.count(key) > 0)

#endif //QL_UTILS_H

