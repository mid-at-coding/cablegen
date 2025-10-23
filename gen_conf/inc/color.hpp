#ifndef COLOR_H
#define COLOR_H

#include <optional>
#include <string>

struct HSV;

struct RGB{
	int r,g,b;
	RGB& operator=(const HSV& in);
	RGB& operator+(const RGB& in);
};

struct HSV{
	int h,s,v;
	HSV& operator=(const RGB& in);
	HSV& operator+=(const HSV& in);
	HSV& operator-=(const HSV& in);
};


// Class to represent a color of any representation
class Color{
	RGB rgb;
	HSV hsv;
	bool _set;
public : 
	std::optional<RGB> getRGB();
	std::optional<HSV> getHSV();
	Color();
	Color(RGB v);
	Color(HSV v);
	void set(RGB v);
	void set(HSV v);
};


std::string RGBtos(const RGB& in);
#endif
