#include "arguments.hpp"

namespace po = boost::program_options;

namespace airglow::arguments {
	std::istream &operator>>(std::istream &in, WindowMode &window_mode) {
		std::string token;
		in >> token;

		boost::to_upper(token);

		if (token == "BORDERLESS") {
			window_mode = WINDOW_BORDERLESS;
		} else if (token == "WINDOWED") {
			window_mode = WINDOW_WINDOWED;
		} else if (token == "FULLSCREEN") {
			window_mode = WINDOW_FULLSCREEN;
		} else {
			throw po::invalid_option_value("");
		}

		return in;
	}

	std::ostream &operator<<(std::ostream &out, WindowMode &window_mode)
	{
		if (window_mode == WINDOW_BORDERLESS) {
			return out << "borderless";
		} else if (window_mode == WINDOW_WINDOWED) {
			return out << "windowed";
		} else if (window_mode == WINDOW_FULLSCREEN) {
			return out << "fullscreen";
		} else {
			throw po::invalid_option_value("");
		}
	}

	void parse_args(int argc, char* argv[], struct options *options) {
		po::options_description desc("Allowed options");
		desc.add_options()
				("help,h", "Produce help message")
				("settings,s", po::value<std::string>(), "Alternative path to settings file")
				(
					"output,o", po::value<WindowMode>(&options->window_mode)->default_value(WINDOW_BORDERLESS),
					R"(Window mode: "windowed", "borderless", "fullscreen")"
				);
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << std::endl;
		}

		if(vm.count("settings")) {
			options->settings = std::make_shared<std::string>(vm["settings"].as<std::string>());
		}
		else {
			options->settings = std::make_shared<std::string>("");
		}
	}
}