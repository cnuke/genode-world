/*
 * \brief  OpenWeatherMap reporter
 * \author Josef Soentgen
 * \date   2018-09-10
 *
 * Loosely inspired by 'fetchurl'.
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <base/log.h>
#include <libc/component.h>
#include <os/path.h>
#include <os/reporter.h>
#include <timer_session/connection.h>

/* cURL includes */
#include <curl/curl.h>


/* dummies to prevent warnings printed by unimplemented libc functions */
extern "C" int   issetugid() { return 1; }
extern "C" pid_t getpid()    { return 1; }


namespace Util {

	size_t next_char(char const *s, size_t start, char const c)
	{
		size_t v = start;
		while (s[v]) {
			if (s[v] == c) { break; }
			v++;
		}
		return v - start;
	}

	size_t mul(size_t a, size_t b)
	{
		return a * b; // XXX {under,over}flow check
	}

	/*
	 * Append interface
	 */
	struct Append
	{
		virtual bool append(char const*, size_t) = 0;
		virtual ~Append() { }
	};

	/*
	 * Validation helper
	 */
	struct Validate
	{
		bool &value;
		Validate(bool &v) : value(v) { value = false; }
		~Validate()                  { value = true; }
	};
} /* namespace Util */



namespace OpenWeatherMap {
	template<size_t> struct Data;
	struct Main;

	using Url = Genode::String<1024>;
	using String = Genode::String<64>;

	enum Units { KELVIN, IMPERIAL, METRIC, };
	using Unit = Genode::String<4>;

	Units units_from_string(String const &s)
	{
		if      (s == "metric")   { return Units::METRIC; }
		else if (s == "imperial") { return Units::IMPERIAL; }
		else                      { return Units::KELVIN; }
	}

	String units_param_from_units(Units units)
	{
		switch (units) {
		case Units::IMPERIAL: return "units=imperial";
		case Units::METRIC:   return "units=metric";
		case Units::KELVIN:
		default:              return "";
		}
	}

	Unit unit_from_units(Units units)
	{
		switch (units) {
		case Units::IMPERIAL: return "°F";
		case Units::METRIC:   return "°C";
		case Units::KELVIN:
		default:              return "°K";
		}
	}
}


template <size_t CAP>
struct OpenWeatherMap::Data : Util::Append
{
	Url const url;

	char   buffer[CAP] { };
	size_t written     { 0 };

	Data(Url const &base,
	     String const &api_key, Units const units, String const &loc)
	:
		url(base, "mode=xml", "&",
		    units_param_from_units(units), "&",
		    loc, "&", "APPID=", api_key)
	{ }

	Genode::Xml_node xml() const
	{
		/* OWM returns <?xml ...>\n<data ...> */
		size_t pos = Util::next_char(buffer, 0, '\n') + 1;
		return { buffer+pos, written - pos };
	}

	void reset() { written = 0; }

	bool append(char const *src, size_t length) override
	{
		if (sizeof(buffer) - 1 < written + length) {
			return false;
		}

		Genode::memcpy(buffer+written, src, length);
		written += length;
		buffer[written] = 0;
		return true;
	}
};


static signed long write_cb(char   *ptr,
                            size_t  size,
                            size_t  nmemb,
                            void   *userdata)
{
	Util::Append &b = *(reinterpret_cast<Util::Append*>(userdata));
	size_t length = Util::mul(size, nmemb);
	return b.append(ptr, length) ? length : -1;
}


struct OpenWeatherMap::Main
{
	Libc::Env &_env;

	/*** data ***/

	Url const base_url { "https://api.openweathermap.org/data/2.5/" };

	enum { CSIZE = 4u << 10, };
	struct Current : Data<CSIZE>
	{
		enum { INVALID_TEMP = -4242l };
		using Name    = Genode::String<64>;
		using Weather = Genode::String<32>;

		Name name { };
		long temp { INVALID_TEMP };
		Unit unit { };
		Weather weather { };

		bool _valid { false };

		Current(Url const &base,
		        String const &api_key, Units const units, String const &loc)
		: Data(Url(base, "weather?"), api_key, units, loc) { }

		void reset()
		{
			_valid = false;
			Data<CSIZE>::reset();
		}

		void update()
		{
			Util::Validate v(_valid);

			Genode::Xml_node node = Data<CSIZE>::xml();
			Genode::Xml_node cnode = node.sub_node("city");
			Genode::Xml_node tnode = node.sub_node("temperature");
			Genode::Xml_node wnode = node.sub_node("weather");

			name    = cnode.attribute_value("name", Name());
			temp    = tnode.attribute_value("value", (long)INVALID_TEMP);
			weather = wnode.attribute_value("value", Weather());

			String unit_str = tnode.attribute_value("unit", String());

			if (!name.valid() || temp == INVALID_TEMP || !unit_str.valid()) { throw -1; }

			Units u = units_from_string(unit_str);
			unit = unit_from_units(u);
		}

		void report(Genode::Xml_generator &xml) const
		{
			xml.node("frame", [&] {
				xml.node("vbox", [&] {
					xml.node("label", [&] {
						xml.attribute("text", name);
					});
					xml.node("hbox", [&] {
						xml.node("label", [&] {
							xml.attribute("text", weather);
						});
						xml.node("label", [&] {
							xml.attribute("text", temp);
						});
						xml.node("label", [&] {
							xml.attribute("text", unit);
						});
			}); }); });
		}

		void print(Genode::Output &out) const
		{
			Genode::print(out, "Current: '", name, "' ",
			              temp, unit, " (", weather, ")");
		}

		bool valid() const { return _valid; }
	};

	Genode::Constructible<Current>  _current { };

	/*** config ***/

	Genode::Attached_rom_dataspace _config_rom { _env, "config" };

	Genode::Signal_handler<OpenWeatherMap::Main> _config_sigh {
		_env.ep(), *this, &OpenWeatherMap::Main::_handle_config_update };

	/**
	 * Handle config update
	 */
	void _handle_config_update()
	{
		using namespace Genode;

		Xml_node config = _config_rom.xml();

		try {
			_config.update(config);

			if (!config.has_sub_node("location")) {
				Genode::error("'location' node missing");
				throw Config::Invalid();
			}

			Genode::Xml_node location = config.sub_node("location");

			String units_str;
			units_str = location.attribute_value("units", units_str);
			Units units = units_from_string(units_str);

			bool use_id    = location.has_attribute("id");
			bool use_query = location.has_attribute("query");

			if (!use_id && !use_query) {
				Genode::error("'location' node invalid");
				throw Config::Invalid();
			}

			/* id is more specific and will be always preferred */
			String query;
			if (use_id) {
				query = String("id=",
				               location.attribute_value("id", String()));
			} else if (use_query) {
				query = String("q=",
				               location.attribute_value("query", String()));
			}

			if (_current.constructed()) { _current.destruct(); }

			bool current  = location.attribute_value("current",  true);

			if (current) {
				_current.construct(base_url, _config.api_key, units, query);
			}

			/*
			 * Trigger new update immediately.
			 */
			_update_timeout.schedule(Microseconds(0));
		} catch (Config::Invalid) {
			Genode::error("invalid config");
			if (_current.constructed()) { _current.destruct(); }
			return;
		}
	}

	struct Config
	{
		struct Invalid : Genode::Exception { };

		bool verbose { false };
		bool log     { false };
		bool report  { true  };

		unsigned update_interval { 60u };

		String api_key { };

		void update(Genode::Xml_node const &node)
		{
			verbose = node.attribute_value("verbose", verbose);
			log     = node.attribute_value("log",     log);
			report  = node.attribute_value("report",  report);

			update_interval = node.attribute_value("update_interval",
			                                       update_interval);

			api_key = node.attribute_value("api_key", String());
			if (!node.has_attribute("api_key") || api_key.length() - 1 != 32) {
				Genode::error("'api_key' invalid");
				throw Invalid();
			}
		}
	};

	Config _config { };

	/*** timer ***/

	Timer::Connection _timer { _env, "update_timer" };

	Timer::One_shot_timeout<Main> _update_timeout {
		_timer, *this, &Main::_update };

	void _update(Genode::Duration)
	{
		_update();

		Genode::Duration to(Genode::Milliseconds(_config.update_interval * 60000));

		if ((to.trunc_to_plain_ms().value > 0) &&
		    (!_update_timeout.scheduled())) {
			_update_timeout.schedule(to.trunc_to_plain_us());
		}
	}

	/**
	 * Update weather data and generate LOG message and Report
	 *
	 * This method is called every N minutes by the timeout handler.
	 */
	void _update()
	{
		Libc::with_libc([&]() {
			CURL *curl = curl_easy_init();
			if (!curl) {
				Genode::error("failed to initialize libcurl");
				return;
			}

			/* set common settings that are used by every call once */
			curl_easy_setopt(curl, CURLOPT_FAILONERROR,    1L);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       true);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

			/* current information */
			if (_current.constructed()) {
				_current->reset();

				curl_easy_setopt(curl, CURLOPT_URL, _current->url.string());
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &*_current);
				CURLcode res = curl_easy_perform(curl);
				if (res != CURLE_OK) {
					Genode::error(curl_easy_strerror(res),
					              ": failed to get current weather");
				} else try {
					_current->update();
				} catch (...) { Genode::error("invalid data"); }
			}

			curl_easy_cleanup(curl);
		});

		bool const ok = _current->valid();
		if (ok && _config.log)    { _log();    }
		if (ok && _config.report) { _report(); }
	}

	/**
	 * Print weather information to LOG
	 */
	void _log()
	{
		if (_current->valid()) { Genode::log(_current); }
	}

	Genode::Expanding_reporter _reporter { _env, "dialog", "dialog" };

	/**
	 * Generate menu_view dialog report
	 */
	void _report()
	{
		_reporter.generate([&] (Genode::Xml_generator &xml) {
			if (_current->valid()) { _current->report(xml); }
		});
	}

	/**
	 * Constructor
	 */
	Main(Libc::Env &e) : _env(e)
	{
		Libc::with_libc([&]() {
			curl_global_init(CURL_GLOBAL_DEFAULT);
		});

		/*
		 * Process inital config, which will trigger initial
		 * weather information query.
		 */
		_config_rom.sigh(_config_sigh);
		_handle_config_update();
	}

	/**
	 * Destructor
	 */
	~Main()
	{
		Libc::with_libc([&]() {
			curl_global_cleanup();
		});
	}
};


void Libc::Component::construct(Libc::Env &env)
{
	static OpenWeatherMap::Main inst(env);
}
