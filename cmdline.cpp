#include "cmdline.h"

namespace cmdline
{
    namespace detail
    {
        std::string demangle(const std::string &name)
        {
#ifdef _MSC_VER
            return name;
#elif defined __GNUC__
            int status = 0;
            char *p = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
            std::string ret(p);
            free(p);
            return ret;
#endif
        }
    }

    cmdline_error::cmdline_error(const std::string &msg)
        : msg(msg)
    {
    }

    cmdline_error::~cmdline_error() = default;

    const char *cmdline_error::what() const
    {
        return msg.c_str();
    }

    parser::parser() = default;

    parser::parser(const parser &obj)
    {
        copy_parser(obj, *this);
    }

    parser::parser(parser &&obj)
    {
        move_parser(obj, *this);
    }

    parser &parser::operator=(const parser &obj)
    {
        copy_parser(obj, *this);
        return *this;
    }

    parser &parser::operator=(parser &&obj)
    {
        move_parser(obj, *this);
        return *this;
    }

    parser::~parser()
    {
        for (auto &item : options)
        {
            delete item.second;
        }

        options.clear();
    }

    void parser::add(const std::string &name,
        char short_name, const std::string &desc)
    {
        if (options.count(name))
        {
            throw cmdline_error("multiple definition: " + name);
        }
        else
        {
            auto opt = new option_without_value(name, short_name, desc);
            options[name] = opt;
            ordered.push_back(opt);
        }
    }

    bool parser::exist(const std::string &name) const
    {
        if (options.count(name))
        {
            return options.find(name)->second->has_set();
        }
        else
        {
            throw cmdline_error("there is no flag: --" + name);
        }
    }

    bool parser::parse_check(const std::string &arg)
    {
        if (!options.count("help"))
        {
            add("help", '?', "print this message");
        }

        return check(0, parse(arg));
    }

    bool parser::parse_check(const std::vector<std::string> &args)
    {
        if (!options.count("help"))
        {
            add("help", '?', "print this message");
        }

        return check(args.size(), parse(args));
    }

    bool parser::parse_check(int argc, char *argv[])
    {
        if (!options.count("help"))
        {
            add("help", '?', "print this message");
        }

        return check(argc, parse(argc, argv));
    }

    std::string parser::usage() const
    {
        std::ostringstream oss;
        oss << "usage: " << prog_name << " ";
        for (const auto &item : ordered)
        {
            if (item->must())
            {
                oss << item->short_description() << " ";
            }
        }

        oss << "[options] ... " << ftr << std::endl;
        oss << "options:" << std::endl;

        size_t max_width = 0;
        for (const auto &item : ordered)
        {
            max_width = std::max<size_t>(max_width, item->name().length());
        }

        for (const auto &item : ordered)
        {
            if (item->short_name())
            {
                oss << "  -" << item->short_name() << ", ";
            }
            else
            {
                oss << "      ";
            }

            oss << "--" << item->name();
            for (size_t j = item->name().length(); j < max_width + 4; ++j)
            {
                oss << ' ';
            }

            oss << item->description() << std::endl;
        }

        return oss.str();
    }

    std::string parser::error_full() const
    {
        std::ostringstream oss;
        for (const auto &item : errors)
        {
            oss << item << std::endl;
        }

        return oss.str();
    }

    bool parser::parse(const std::string &arg)
    {
        std::vector<std::string> args;

        std::string buf;
        bool in_quote = false;
        for (std::string::size_type i = 0; i<arg.length(); i++)
        {
            if (arg[i] == '\"')
            {
                in_quote = !in_quote;
                continue;
            }

            if (arg[i] == ' ' && !in_quote)
            {
                args.push_back(buf);
                buf = "";
                continue;
            }

            if (arg[i] == '\\')
            {
                i++;
                if (i >= arg.length())
                {
                    errors.push_back("unexpected occurrence of '\\' at end of string");
                    return false;
                }
            }

            buf += arg[i];
        }

        if (in_quote)
        {
            errors.push_back("quote is not closed");
            return false;
        }

        if (buf.length() > 0)
        {
            args.push_back(buf);
        }

        for (const auto &item : args)
        {
            std::cout << "\"" << item << "\"" << std::endl;
        }

        return parse(args);
    }

    bool parser::parse(const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            return false;
        }

        int argc = static_cast<int>(args.size());
        std::vector<const char*> argv(argc);
        for (int i = 0; i < argc; i++)
        {
            argv[i] = args[i].c_str();
        }

        return parse(argc, argv.data());
    }

    bool parser::parse(int argc, const char *const argv[])
    {
        clear();

        if (argc < 1)
        {
            return false;
        }

        if (prog_name == "")
        {
            prog_name = argv[0];
        }

        std::map<char, std::string> lookup;
        for (const auto &item : options)
        {
            if (item.first.empty())
            {
                continue;
            }

            char initial = item.second->short_name();
            if (initial)
            {
                if (lookup.count(initial) != 0)
                {
                    lookup[initial] = "";
                    errors.push_back(std::string("short option '") + initial + "' is ambiguous");
                    return false;
                }
                else
                {
                    lookup[initial] = item.first;
                }
            }
        }

        constexpr std::size_t short_dash_len = 1;
        constexpr std::size_t long_dash_len = 2;
        for (int i = 1; i < argc; i++)
        {
            if (strncmp(argv[i], "--", long_dash_len) == 0)
            {
                const char *p = strchr(argv[i] + long_dash_len, '=');
                if (p)
                {
                    std::string name(argv[i] + long_dash_len, p);
                    std::string val(p + 1);
                    set_option(name, val);
                }
                else
                {
                    std::string name(argv[i] + long_dash_len);
                    if (!options.count(name))
                    {
                        errors.push_back("undefined option: --" + name);
                        continue;
                    }
                    if (options[name]->has_value())
                    {
                        if (i + 1 >= argc)
                        {
                            errors.push_back("option needs value: --" + name);
                            continue;
                        }
                        else
                        {
                            i++;
                            set_option(name, argv[i]);
                        }
                    }
                    else
                    {
                        set_option(name);
                    }
                }
            }
            else if (strncmp(argv[i], "-", short_dash_len) == 0)
            {
                char last = argv[i][short_dash_len];
                if (last == '\0')
                {
                    continue;
                }

                for (int j = short_dash_len + 1; argv[i][j] != '\0'; ++j)
                {
                    last = argv[i][j];
                    if (!lookup.count(argv[i][j - short_dash_len]))
                    {
                        errors.push_back(std::string("undefined short option: -") + argv[i][j - short_dash_len]);
                        continue;
                    }

                    if (lookup[argv[i][j - short_dash_len]] == "")
                    {
                        errors.push_back(std::string("ambiguous short option: -") + argv[i][j - short_dash_len]);
                        continue;
                    }

                    set_option(lookup[argv[i][j - short_dash_len]]);
                }

                if (!lookup.count(last))
                {
                    errors.push_back(std::string("undefined short option: -") + last);
                    continue;
                }

                if (lookup[last] == "")
                {
                    errors.push_back(std::string("ambiguous short option: -") + last);
                    continue;
                }

                if (i + 1 < argc && options[lookup[last]]->has_value())
                {
                    set_option(lookup[last], argv[i + 1]);
                    i++;
                }
                else
                {
                    set_option(lookup[last]);
                }
            }
            else
            {
                others.push_back(argv[i]);
            }
        }

        for (const auto &item : options)
        {
            if (!item.second->valid())
            {
                errors.push_back("need option: --" + std::string(item.first));
            }
        }

        return errors.size() == 0;
    }

    bool parser::check(int argc, bool ok)
    {
        if ((argc == 1 && !ok) || exist("help"))
        {
            std::cerr << usage();
            return false;
        }
        else if (!ok)
        {
            std::cerr << error() << std::endl << usage();
            return false;
        }
        else
        {
            return true;
        }
    }

    void parser::set_option(const std::string &name)
    {
        if (!options.count(name))
        {
            errors.push_back("undefined option: --" + name);
        }
        else if (!options[name]->set())
        {
            errors.push_back("option needs value: --" + name);
        }
    }

    void parser::set_option(const std::string &name, const std::string &value)
    {
        if (!options.count(name))
        {
            errors.push_back("undefined option: --" + name);
        }
        else if (!options[name]->set(value))
        {
            errors.push_back("option value is invalid: --" + name + "=" + value);
        }
    }

    void parser::clear()
    {
        for (auto &item : options)
        {
            item.second->clear();
        }

        errors.clear();
        others.clear();
    }

    void parser::move_parser(parser &src, parser &dest)
    {
        dest.options = std::move(src.options);
        dest.ordered = std::move(src.ordered);
        dest.ftr = std::move(src.ftr);
        dest.prog_name = std::move(src.prog_name);
        dest.others = std::move(src.others);
        dest.errors = std::move(src.errors);

        src.options.clear();
        src.ordered.clear();
        src.others.clear();
        src.errors.clear();
    }

    void parser::copy_parser(const parser &src, parser &dest)
    {
        for (const auto &item : src.options)
        {
            auto new_opt = item.second->clone();
            dest.options[item.first] = new_opt;
            dest.ordered.push_back(new_opt);
        }

        dest.ftr = src.ftr;
        dest.prog_name = src.prog_name;
        dest.others = src.others;
        dest.errors = src.errors;
    }

    parser::option_base::option_base() = default;

    parser::option_base::~option_base() = default;

    parser::option_without_value::option_without_value(
        const std::string &name,
        char short_name,
        const std::string &desc)
        : nam(name), snam(short_name), desc(desc), has(false)
    {
    }

    parser::option_base *parser::option_without_value::clone() const
    {
        auto new_opt = new option_without_value(nam, snam, desc);
        new_opt->has = has;
        return new_opt;
    }

    bool parser::option_without_value::has_value() const
    {
        return false;
    }

    void parser::option_without_value::clear()
    {
        has = false;
    }

    bool parser::option_without_value::set()
    {
        has = true;
        return true;
    }

    bool parser::option_without_value::set(const std::string &)
    {
        return false;
    }

    bool parser::option_without_value::has_set() const
    {
        return has;
    }

    bool parser::option_without_value::valid() const
    {
        return true;
    }

    bool parser::option_without_value::must() const
    {
        return false;
    }

    const std::string &parser::option_without_value::name() const
    {
        return nam;
    }

    char parser::option_without_value::short_name() const
    {
        return snam;
    }

    const std::string &parser::option_without_value::description() const
    {
        return desc;
    }

    std::string parser::option_without_value::short_description() const
    {
        return "--" + nam;
    }
}