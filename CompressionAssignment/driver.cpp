/*******************************************************
Image compression via run-length encoding
by Michael Priest

Based on framework provided by Dr James Ferryman
*******************************************************/

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "PPM.h"

using namespace std;
using std::cout;
using std::cin;
using std::endl;

//round value to nearest input, used for reducing colour range
int round(int toRound, int multiple)
{
	return (toRound + (multiple / 2)) / multiple * multiple;
}

//select user input PPM and load it
void select_PPM(_ppm &PPM) {
	string input;
	cout << "Input file name:";
	getline(cin, input);
	cout << "Images\\" << input << ".PPM" << endl;
	string result = "Images\\" + input + ".PPM";
	if (PPM.load_ppm(result.c_str()) == -1) //check for loading error
	{
		cout << "!!! Error while loading image" << endl << endl;
	}
	else {
		cout << "Image " << input << " loaded succesfully" << endl;
		cout << "Size of image: " << PPM.get_image_width() << "x" << PPM.get_image_height() << endl;
	}
}

void write_compressed_file(int width, int height, vector<int> r, vector<int> g, vector<int> b, string filename, int compress_factor) {
	string savename = "Images\\" + filename + ".rle";
	ofstream outfile(savename);
	cout << "writing to file..." << endl;
	outfile << width << '\n';
	outfile << height << '\n';
	for (int i = 0; i < r.size(); i++) {
		outfile << r[i] << " ";
	}
	outfile << "\n";
	for (int i = 0; i < g.size(); i++) {
		outfile << g[i] << " ";
	}
	outfile << "\n";
	for (int i = 0; i < b.size(); i++) {
		outfile << b[i] << " ";
	}
	cout << "succesfully written, saving as: " + savename << endl;
}

//encode data via run-length encoding
void encode(vector<int> &input, vector<int> &output, string channel) {
	cout << "encoding " + channel + " channel..." << endl;
	int run_length;
	int j = 0;

	for (int i = 0; i < input.size(); i++) {
		if (input[i] == 256) { input[i] = 255; }
	}

	//loop through 1d stream of pixel values
	for (int i = 0; i < input.size(); i++) {

		//add pixel intesity to vector
		output.resize(j + 1);
		output[j++] = input[i];

		//count occurences of same pixel value
		run_length = 1;
		while (i + 1 < input.size() && (input[i] == input[i + 1])) {
			run_length++;
			i++;
		}

		//add pixel count to vector if more than 1
		if (run_length > 1) {
			output.resize(j + 1);
			output[j++] = run_length+255;
		}

	}
}

//start compression process
void compress(_ppm &PPM, string savename, int compress_factor) {

	vector<int> red1D, green1D, blue1D, red_encoded, green_encoded, blue_encoded;

	//convert 2d image of pixels to a 1d stream
	for (int i = 0; i < PPM.get_image_width(); i++) {
		for (int j = 0; j < PPM.get_image_height(); j++) {
			red1D.push_back(PPM.get_pixel(i, j, RED));
			green1D.push_back(PPM.get_pixel(i, j, GREEN));
			blue1D.push_back(PPM.get_pixel(i, j, BLUE));
		}
	}

	//compression factor
	cout << "reducing pixel intesities to range of " << compress_factor << "..." << endl;
	for (int i = 0; i < red1D.size(); i++) {
		red1D[i] = round(red1D[i], 256/compress_factor);
	}
	for (int i = 0; i < green1D.size(); i++) {
		green1D[i] = round(green1D[i], 256/compress_factor);
	}
	for (int i = 0; i < blue1D.size(); i++) {
		blue1D[i] = round(blue1D[i], 256/compress_factor);
	}

	//encode each colour channel separately
	encode(red1D, red_encoded, "red");
	encode(green1D, green_encoded, "green");
	encode(blue1D, blue_encoded, "blue");

	//write data to output file fully encoded and compressed
	write_compressed_file(PPM.get_image_width(), PPM.get_image_height(), red_encoded, green_encoded, blue_encoded, savename, compress_factor);
}

//output metrics to console
void metrics(string filename, vector<int> decom_r, vector<int> decom_g, vector<int> decom_b) {
	
	ifstream file;
	file.open(filename);
	//discard width & height
	int temp;
	int com_sum = 0;
	file >> temp;
	file >> temp;
	while (file >> temp) {
		com_sum++;
	}
	file.close();

	//sum of decompressed values
	int decom_sum = decom_r.size() + decom_g.size() + decom_b.size();

	int diff = decom_sum - com_sum;
	float decrease = ((float)diff / (float)decom_sum) * 100;

	cout << endl << "COMPRESSION STATISTICS" << endl;
	cout << "Compressed size: " << com_sum << endl;
	cout << "Decompressed size: " << decom_sum << endl;
	cout << "Reduction percentage: " << decrease << "%" << endl;
}

//edit decompressed vector to make it suitable for writing to file
void manipulate_vector(vector<int> input, vector<int> &output, int width, int height) {
	//decoded arrays needs to be modified
	//so it can be written to file.
	//example of process shown below:
	// 1 2 3       1 4 7
	// 4 5 6  -->  2 5 8
	// 7 8 9       3 6 9
	int i = 0;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			output[width*y + x] = input[i];
			i++;
		}
	}
}

void write_decompressed_file(vector<int> r, vector<int> g, vector<int> b, int width, int height, string savename) {
	ofstream outfile("Images\\" + savename + ".ppm");
	cout << "writing to file..." << endl;
	outfile << "P3\n\n" << width << " " << height << "\n255\n";
	for (int i = 0; i < r.size(); i++) {
		outfile << r[i] << "  " << g[i] << "  " << b[i] << "    ";
		if ((i + 1) % width == 0) {
			outfile << "\n";
		}
	}
	outfile.close();
}

//start decompression process
void decompress(string loadname, string savename) {

	int width, height;

	//open file
	ifstream file;
	file.open("Images\\" + loadname + ".rle");
	if (!file.is_open()) {
		cout << "no such file" << endl;
		return;
	}

	//get image dimensions from header of file
	file >> width;
	file >> height;

	//store decoded data in vectors, separate vector for each colour channel
	vector<int> r, g, b, r2, g2, b2;
	int i = 0, temp, count, a;
	bool control = true;

	//Decode all three colour channels separetly storing in separate vectors
	cout << "decoding red channel..." << endl;
	while (control) {
		if (i >= width * height) {
			control = false;
		} else {
			file >> a;
			if (a > 255) {
				count = a - 255;
				for (int j = 0; j < count - 1; j++) {
					r.resize(i + 1);
					r[i++] = temp;
				}
			}
			else {
				r.resize(i + 1);
				r[i] = a;
				temp = r[i++];
			}

		}
	}
	i = 0;
	control = true;
	cout << "decoding green channel..." << endl;
	while (control) {
		if (i >= width * height) {
			control = false;
		}
		else {
			file >> a;
			if (a > 255) {
				count = a - 255;
				for (int j = 0; j < count - 1; j++) {
					g.resize(i + 1);
					g[i++] = temp;
				}
			}
			else {
				g.resize(i + 1);
				g[i] = a;
				temp = g[i++];
			}

		}
	}
	i = 0;
	control = true;
	cout << "decoding blue channel..." << endl;
	while (control) {
		if (i >= width * height) {
			control = false;
		}
		else {
			file >> a;
			if (a > 255) {
				count = a - 255;
				for (int j = 0; j < count - 1; j++) {
					b.resize(i + 1);
					b[i++] = temp;
				}
			}
			else {
				b.resize(i + 1);
				b[i] = a;
				temp = b[i++];
			}

		}
	}

	file.close();

	//copy vector 1 into vector 2 so both are the same size
	r2 = r; g2 = g; b2 = b;
	manipulate_vector(r, r2, width, height);
	manipulate_vector(g, g2, width, height);
	manipulate_vector(b, b2, width, height);

	//write ouput to PPM file
	write_decompressed_file(r2, g2, b2, width, height, savename);
	cout << "successfully written, saving as: Images\\" << savename << ".ppm" << endl;

	//Output metrics
	metrics("Images\\" + loadname + ".rle", r2, g2, b2);
}

//converting P6 PPM types to P3 PPM types
void convert_P6_to_P3() {
	_ppm PPM;
	PPM.load_ppm("Images\\2.ppm");
	ofstream file;
	file.open("Images\\2-p3.ppm");

	vector<int> red1D, green1D, blue1D;

	//convert 2d image of pixels to a 1d stream
	for (int i = 0; i < PPM.get_image_height(); i++) {
		for (int j = 0; j < PPM.get_image_width(); j++) {

			file << PPM.get_pixel(j, i, RED) << "  ";
			file << PPM.get_pixel(j, i, GREEN) << "  ";
			file << PPM.get_pixel(j, i, BLUE) << "    ";
		}
		file << "\n";
	}

	file.close();
}

int main() {

	bool control = true;
	char option;
	int compression_factor;
	string save, load, menu = "Commands: (L)oad PPM, (C)ompress, (D)ecompress, (H)elp, (E)xit";
	_ppm PPM;
	cout << "Image compression through run length encoding" << endl << "by Michael Priest" << endl;;
	cout << menu << endl;
	while (control) {

		cout << endl << "Input next command" << endl;
		cin >> option;
		cin.ignore();

		switch (option) {
			//EXIT
			case 'e':
				cout << "Exiting program..." << endl;
				control = false;
				break;

			//SELECT AND LOAD PPM
			case 'l':
				select_PPM(PPM);
				break;

			//COMPRESS WITH RUN LENGHT ENCODING
			case 'c':
				cout << "Save compressed file as:" << endl;
				cin >> save;
				cin.ignore();
				cout << "Number of color intensities: (Recommended 8, 16, 32, 64, 256)" << endl;
				cin >> compression_factor;
				cin.ignore();
				if (PPM.get_load_status()) {
					compress(PPM, save, compression_factor);
				}
				else {
					cout << "No PPM image loaded to compress" << endl;
				}
				break;

			//DECOMPRESS
			case 'd':
				cout << "File to decompress:" << endl;
				cin >> load;
				cin.ignore();
				cout << "Save decompressed file as:" << endl;
				cin >> save;
				cin.ignore();
				decompress(load, save);
				break;

			//CONVERT P6 PPM TO P3 PPM
			case 'p':
				convert_P6_to_P3();
				break;

			//HELP
			case 'h':
				cout << menu << endl;
				break;
		}

	}

	return 0;
}


/*
PPM FORMAT

P3
<width> <height>
<max-col-val>
 <r>  <g>  <b>    <r>  <g>  <b>
 <r>  <g>  <b>    <r>  <g>  <b>
 <r>  <g>  <b>    <r>  <g>  <b>

*/