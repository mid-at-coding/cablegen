#include "../inc/color.hpp"
#include <cmath>
#include <optional>

static int addHue(const int h1, const int h2){
	if(h1 + h2 > 360)
		return std::abs(h1 - h2);
	if(h1 + h2 < 0)
		return 360 - std::abs(h1 - h2);
	return h1 + h2;
}

static int addVal(const int v1, const int v2){
	if(v1 + v2 > 100)
		return 100;
	if(v1 + v2 < 0)
		return 0;
	return v1 + v2;
}

std::optional<RGB> Color::getRGB(){
	if(!_set)
		return {};
	return {rgb};
}
std::optional<HSV> Color::getHSV(){
	if(!_set)
		return {};
	return {hsv};
}

HSV& HSV::operator+=(const HSV& in){
	// hue
	h = addHue(h, in.h);
	
	// saturation
	s = addVal(s, in.s);

	// value
	v = addVal(v, in.v);

	return (*this);
}

HSV& HSV::operator-=(const HSV& in){
	// hue
	h = addHue(h, -in.h);
	
	// saturation
	s = addVal(s, -in.s);

	// value
	v = addVal(v, -in.v);
	return(*this);
}

void Color::set(RGB v){
	_set = true;
	rgb = v;
	hsv = v;
}
void Color::set(HSV v){
	_set = true;
	rgb = v;
	hsv = v;
}

Color::Color(HSV v){
	set(v);
}

Color::Color(RGB v){
	set(v);
}

Color::Color(){

}


std::string RGBtos(const RGB& in){
	return std::to_string(in.r) + ", " + std::to_string(in.g) + ", " + std::to_string(in.b);
}
