#pragma once
/*
    Copyright (c) 2009, Hideyuki Tanaka
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY <copyright holder> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
    New feature by chenzs108:
     1. Support Microsoft Visual C++
     2. Support clear history and reparse new args
     3. Support object copy, move and assignment
*/

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <algorithm>
#ifdef __GNUC__
#include <cxxabi.h>
#endif
#include <cstdlib>


namespace cmdline
{
    namespace detail
    {
        template <typename T1, typename T2>
        struct is_same
        {
            static const bool value = false;
        };

        template <typename T>
        struct is_same<T, T>
        {
            static const bool value = true;
        };

        template <typename Target, typename Source, bool Same>
        class lexical_cast_t
        {
        public:
            static Target cast(const Source &arg)
            {
                Target ret;
                std::stringstream ss;
                if (!(ss << arg && ss >> ret && ss.eof()))
                {
                    throw std::bad_cast();
                }

                return ret;
            }
        };

        template <typename Target, typename Source>
        class lexical_cast_t<Target, Source, true>
        {
        public:
            static Target cast(const Source &arg)
            {
                return arg;
            }
        };

        template <typename Source>
        class lexical_cast_t<std::string, Source, false>
        {
        public:
            static std::string cast(const Source &arg)
            {
                std::ostringstream ss;
                ss << arg;
                return ss.str();
            }
        };

        template <typename Target>
        class lexical_cast_t<Target, std::string, false>
        {
        public:
            static Target cast(const std::string &arg)
            {
                Target ret;
                std::istringstream ss(arg);
                if (!(ss >> ret && ss.eof()))
                {
                    throw std::bad_cast();
                }

                return ret;
            }
        };

        template<typename Target, typename Source>
        Target lexical_cast(const Source &arg)
        {
            return lexical_cast_t<Target, Source, detail::is_same<Target, Source>::value>::cast(arg);
        }

        std::string demangle(const std::string &name);

        template <class T>
        std::string readable_typename()
        {
            return demangle(typeid(T).name());
        }

        template <class T>
        std::string default_value(T def)
        {
            return detail::lexical_cast<std::string>(def);
        }

        template <>
        inline std::string readable_typename<std::string>()
        {
            return "string";
        }
    }

    class cmdline_error : public std::exception
    {
    public:
        cmdline_error(const std::string &msg);
        ~cmdline_error();
        const char *what() const;

    private:
        std::string msg;
    };

    template <class T>
    struct default_reader
    {
        T operator()(const std::string &str)
        {
            return detail::lexical_cast<T>(str);
        }
    };

    template <class T>
    struct range_reader
    {
        range_reader(const T &low, const T &high)
            : low(low), high(high)
        {
        }

        T operator()(const std::string &s) const
        {
            T ret = default_reader<T>()(s);
            if (!(ret >= low && ret <= high))
            {
                throw cmdline::cmdline_error("range_error");
            }

            return ret;
        }

    private:
        T low, high;
    };

    template <class T>
    range_reader<T> range(const T &low, const T &high)
    {
        return range_reader<T>(low, high);
    }

    template <class T>
    struct oneof_reader
    {
        T operator()(const std::string &s)
        {
            T ret = default_reader<T>()(s);
            if (std::find(alt.begin(), alt.end(), ret) == alt.end())
            {
                throw cmdline_error("");
            }

            return ret;
        }

        void add(const T &v)
        {
            alt.push_back(v);
        }

    private:
        std::vector<T> alt;
    };

    template <typename Reader, typename T>
    Reader oneof_impl(Reader& reader, T last)
    {
        reader.add(last);
        return reader;
    }

    template <typename Reader, typename T, typename... Rest>
    Reader oneof_impl(Reader& reader, T&& first, Rest&&... rest)
    {
        reader.add(first);
        return oneof_impl(reader, rest...);
    }

    template <typename T, typename... Rest>
    oneof_reader<T> oneof(Rest&&... rest)
    {
        oneof_reader<T> reader;
        return oneof_impl(reader, rest...);
    }

    class parser
    {
    public:
        parser();
        parser(const parser &obj);
        parser(parser &&obj);
        parser& operator=(const parser &obj);
        parser& operator=(parser &&obj);
        ~parser();

        void add(const std::string &name, char short_name = '\0',
            const std::string &desc = "");

        template <class T>
        void add(const std::string &name, char short_name = '\0',
            const std::string &desc = "", bool need = true, const T def = T())
        {
            add(name, short_name, desc, need, def, default_reader<T>());
        }

        template <class T, class F>
        void add(const std::string &name, char short_name = '\0',
            const std::string &desc = "", bool need = true, const T def = T(), F reader = F())
        {
            if (options.count(name))
            {
                throw cmdline_error("multiple definition: " + name);
            }
            else
            {
                options[name] = new option_with_value_with_reader<T, F>(name, short_name, need, def, desc, reader);
                ordered.push_back(options[name]);
            }
        }

        void footer(const std::string &f)
        {
            ftr = f;
        }

        void set_program_name(const std::string &name)
        {
            prog_name = name;
        }

        bool exist(const std::string &name) const;

        template <class T>
        const T &get(const std::string &name) const
        {
            if (options.count(name))
            {
                const option_with_value<T> *p = dynamic_cast<const option_with_value<T>*>(options.find(name)->second);
                if (p != nullptr)
                {
                    return p->get();
                }
                else
                {
                    throw cmdline_error("type mismatch flag '" + name + "'");
                }
            }
            else
            {
                throw cmdline_error("there is no flag: --" + name);
            }
        }

        const std::vector<std::string> &rest() const
        {
            return others;
        }

        bool parse(const std::string &arg);

        bool parse(const std::vector<std::string> &args);

        bool parse(int argc, const char * const argv[]);

        bool parse_check(const std::string &arg);

        bool parse_check(const std::vector<std::string> &args);

        bool parse_check(int argc, char *argv[]);

        void clear();

        std::string error() const
        {
            return errors.size()>0 ? errors[0] : "";
        }

        std::string error_full() const;

        std::string usage() const;

    private:
        static void move_parser(parser &src, parser &dest);

        static void copy_parser(const parser &src, parser &dest);

        bool check(int argc, bool ok);

        void set_option(const std::string &name);

        void set_option(const std::string &name, const std::string &value);

        class option_base
        {
        public:
            option_base();
            virtual ~option_base();

            virtual option_base *clone() const = 0;
            virtual bool has_value() const = 0;
            virtual bool set() = 0;
            virtual void clear() = 0;
            virtual bool set(const std::string &value) = 0;
            virtual bool has_set() const = 0;
            virtual bool valid() const = 0;
            virtual bool must() const = 0;
            virtual const std::string &name() const = 0;
            virtual char short_name() const = 0;
            virtual const std::string &description() const = 0;
            virtual std::string short_description() const = 0;
        };

        class option_without_value : public option_base
        {
        public:
            option_without_value(
                const std::string &name,
                char short_name,
                const std::string &desc);

            virtual option_base *clone() const override;
            virtual bool has_value() const override;
            virtual bool set() override;
            virtual void clear() override;
            virtual bool set(const std::string &) override;
            virtual bool has_set() const override;
            virtual bool valid() const override;
            virtual bool must() const override;
            virtual const std::string &name() const override;
            virtual char short_name() const override;
            virtual const std::string &description() const override;
            virtual std::string short_description() const override;

        private:
            std::string nam;
            char snam;
            std::string desc;
            bool has;
        };

        template <class T>
        class option_with_value : public option_base
        {
        public:
            option_with_value(
                const std::string &name,
                char short_name,
                bool need,
                const T &def,
                const std::string &desc)
                : nam(name), snam(short_name), need(need), has(false)
                , def(def), actual(def)
            {
                this->desc = full_description(desc);
            }

            virtual const T &get() const
            {
                return actual;
            }

            virtual bool has_value() const override
            {
                return true;
            }

            virtual void clear() override
            {
                actual = def;
                has = false;
            }

            virtual bool set() override
            {
                return false;
            }

            virtual bool set(const std::string &value) override
            {
                try
                {
                    actual = read(value);
                    has = true;
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            }

            virtual bool has_set() const override
            {
                return has;
            }

            virtual bool valid() const override
            {
                return (need && !has) ? false : true;
            }

            virtual bool must() const override
            {
                return need;
            }

            virtual const std::string &name() const override
            {
                return nam;
            }

            virtual char short_name() const override
            {
                return snam;
            }

            virtual const std::string &description() const override
            {
                return desc;
            }

            virtual std::string short_description() const override
            {
                return "--" + nam + "=" + detail::readable_typename<T>();
            }

        protected:
            std::string full_description(const std::string &desc)
            {
                return desc + " (" + detail::readable_typename<T>() +
                    (need ? "" : " [=" + detail::default_value<T>(def) + "]")
                    + ")";
            }

            virtual T read(const std::string &s) = 0;

            std::string nam;
            char snam;
            bool need;
            std::string desc;
            bool has;
            T def;
            T actual;
        };

        template <class T, class F>
        class option_with_value_with_reader : public option_with_value<T>
        {
        public:
            option_with_value_with_reader(const std::string &name,
                char short_name, bool need, const T def,
                const std::string &desc, F reader)
                : option_with_value<T>(name, short_name, need, def, desc), reader(reader)
            {
            }

            virtual option_base *clone() const override
            {
                auto new_opt = new option_with_value_with_reader<T, F>(
                    this->nam, this->snam, this->need, this->def, this->desc, this->reader);
                new_opt->has = this->has;
                new_opt->actual = this->actual;
                return new_opt;
            }

        private:
            T read(const std::string &s)
            {
                return reader(s);
            }

            F reader;
        };

        std::map<std::string, option_base*> options;
        std::vector<option_base*> ordered;
        std::string ftr;
        std::string prog_name;
        std::vector<std::string> others;
        std::vector<std::string> errors;
    };
}