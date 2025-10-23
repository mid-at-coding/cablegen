#include "../inc/formatting.hpp"
#include <optional>

std::string colorString(std::string msg, Color color, bool background){
	RGB rgb;
	try{
		rgb = color.getRGB().value();
	}
	catch(std::bad_optional_access){
		return msg; // ok
	}
	return (std::string("\e[") + (background ? "4" : "3") + "8;2;" + std::to_string(rgb.r) + ";" + std::to_string(rgb.g) 
			+ ";" + std::to_string(rgb.b) + "m" + msg + "\e[0m");
}
