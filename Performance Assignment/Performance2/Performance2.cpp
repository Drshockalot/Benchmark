// Performance2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "cimage_pixel_access_opt.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <thread>


// Timer - used to established precise timings for code.
class TIMER
{
	LARGE_INTEGER t_;

	__int64 current_time_;

	public:
		TIMER()	// Default constructor. Initialises this timer with the current value of the hi-res CPU timer.
		{
			QueryPerformanceCounter(&t_);
			current_time_ = t_.QuadPart;
		}

		TIMER(const TIMER &ct)	// Copy constructor.
		{
			current_time_ = ct.current_time_;
		}

		TIMER& operator=(const TIMER &ct)	// Copy assignment.
		{
			current_time_ = ct.current_time_;
			return *this;
		}

		TIMER& operator=(const __int64 &n)	// Overloaded copy assignment.
		{
			current_time_ = n;
			return *this;
		}

		~TIMER() {}		// Destructor.

		static __int64 get_frequency()
		{
			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency); 
			return frequency.QuadPart;
		}

		__int64 get_time() const
		{
			return current_time_;
		}

		void get_current_time()
		{
			QueryPerformanceCounter(&t_);
			current_time_ = t_.QuadPart;
		}

		inline bool operator==(const TIMER &ct) const
		{
			return current_time_ == ct.current_time_;
		}

		inline bool operator!=(const TIMER &ct) const
		{
			return current_time_ != ct.current_time_;
		}

		__int64 operator-(const TIMER &ct) const		// Subtract a TIMER from this one - return the result.
		{
			return current_time_ - ct.current_time_;
		}

		inline bool operator>(const TIMER &ct) const
		{
			return current_time_ > ct.current_time_;
		}

		inline bool operator<(const TIMER &ct) const
		{
			return current_time_ < ct.current_time_;
		}

		inline bool operator<=(const TIMER &ct) const
		{
			return current_time_ <= ct.current_time_;
		}

		inline bool operator>=(const TIMER &ct) const
		{
			return current_time_ >= ct.current_time_;
		}
};

CWinApp theApp;  // The one and only application object

////
//	@param i
//		- The image to brighten
////
void Brighten(CImage *i)
{
	// Get the dimentions of the image
	// creating local variables so these functions do not have to be 
	// called each iteration of the loops
	int width(i->GetWidth());
	int height(i->GetHeight());
	
	// Use a specialized access class to for faster access to each
	// CImage pixel
	CImagePixelAccessOptimizer quickly(i);

	for (int y(0); y < height; ++y)
	{
		for (int x(0); x < width; ++x)
		{	
			// Current pixel for manipulation
			COLORREF pixel = quickly.GetPixel(x, y);

			// Create local copies of the RGB values due to
			// them being used more than once
			BYTE r((pixel >> 16) & 0xff);
			BYTE g((pixel >> 8) & 0xff);
			BYTE b(pixel & 0xff);

			// Update the pixel information with the updated RGB values
			quickly.SetPixel(x, y, RGB(r < 245 ? r + 10 : 255, 
									   g < 245 ? g + 10 : 255, 
									   b < 245 ? b + 10 : 255));
		}
	}
}

////
//	@param i
//		- The image to rotate
////
void Rotate(CImage* i)
{
	// Get the bit depth for the image (Bits Per Pixel)
	int bpp = i->GetBPP();

	// Retrieve the pixel format for the image, for use in a Gdi+ function later
	HBITMAP hbmp = i->Detach();
	Gdiplus::Bitmap *bmpTemp = Gdiplus::Bitmap::FromHBITMAP(hbmp, 0);
	Gdiplus::PixelFormat pixel_format = bmpTemp->GetPixelFormat();
	if (bpp == 32)
		pixel_format = PixelFormat32bppARGB;
	i->Attach(hbmp);

	// Create a Gdi+ bitmap that is a copy of the image
	Gdiplus::Bitmap bmp(i->GetWidth(), i->GetHeight(), i->GetPitch(), pixel_format, static_cast<BYTE*>(i->GetBits()));
	
	// Use Gdi+ built in functionality to rotate the image 90 degrees clockwise
	bmp.RotateFlip(Gdiplus::Rotate90FlipNone);
	
	// Destroy what is currently inside of the original image 
	// so that the newly rotated image can be drawn there instead
	i->Destroy();

	// Only proceed attempting to draw if ATL can sucessfully create a canvas
	// on which to draw
	if(i->Create(bmp.GetWidth(), bmp.GetHeight(), 32, CImage::createAlphaChannel))
	{
		// Convert the Gdi+ bitmap into a Graphics object which can be drawn,
		// and draw it to the original image space
		// (which now has a canvas correctly rotated
		Gdiplus::Bitmap dst(i->GetWidth(), i->GetHeight(), i->GetPitch(), PixelFormat32bppARGB, static_cast<BYTE*>(i->GetBits()));
		Gdiplus::Graphics graphics(&dst);
		graphics.DrawImage(&bmp, 0, 0);
	}
}

////
//	@param i
//		- The image to perform the greyscale conversion
////
void Greyscale(CImage* i)
{
	// Get the dimentions of the image
	// creating local variables so these functions do not have to be 
	// called each iteration of the loops
	int width(i->GetWidth());
	int height(i->GetHeight());

	// Use a specialized access class to for faster access to each
	// CImage pixel
	CImagePixelAccessOptimizer quickly(i);

	// Loop through the image pixels
	// modifying the colour information appropriately
	for (int y(0); y < height; ++y)
	{
		for (int x(0); x < width; ++x)
		{
			// Current pixel for manipulation
			COLORREF pixel = quickly.GetPixel(x, y);
			
			// Using the 'Average' greyscale algorithm, pull each RGB colour element from the pixel
			// and divide by 3, giving an appropriate greyscale colour
			BYTE grey((((pixel >> 16) & 0xff) + ((pixel >> 8) & 0xff) + (pixel & 0xff)) / 3);

			// Update the pixel information with the new grey colour
			quickly.SetPixel(x, y, RGB(grey, grey, grey));
		}
	}
}

////
// @param i
//		- The image to resize
////
CImage* Resize(CImage* i)
{
	// Get the new sizes of the image
	// (All of the images supplied were of even proportions
	//  so it was sufficient to just divide by 2
	int scaledWidth(i->GetWidth() / 2);
	int scaledHeight(i->GetHeight() / 2);

	// It is required to make a duplicate CImage object
	// (which is later deleted correctly in the ProcessImage function)
	// to make a copy
	CImage *copy = new CImage;
	copy->Create(scaledWidth, scaledHeight, i->GetBPP());

	// Use the built in ATL functionality to create a copy of the original image
	// in the specified new dimentions
	i->StretchBlt(copy->GetDC(), 0, 0, scaledWidth, scaledHeight, SRCCOPY);
	copy->ReleaseDC();

	// Return the copy instead of the original image
	// allowing it to be assigned for use in ProcessImage
	return copy;
}

////
//	@param src
//		- The original image
//
//	@param dest
//		- The resized image
////
void BilinearFilter(CImage* src, CImage* dest)
{
	// Use the dimentions of the resized image
	// as the loop counters
	int new_width(dest->GetWidth());
	int new_height(dest->GetHeight());

	// Ascertain the resize ratios for the new image
	// For the purposes of the given images, being since the the new images are 50% smaller
	// than the originals, this calculation is almost certain to end in "2" for each calculation,
	// meaning 2 full pixels on both the x and y axis' are used to create 1 pixel for the resized image
	int resize_ratio_x(src->GetWidth() / new_width);
	int resize_ratio_y(src->GetHeight() / new_height);
	
	// Create two copies of the specialized pixel access class,
	// as both the src and dest images will be used during the algorithm
	CImagePixelAccessOptimizer quickly_dest(dest);
	CImagePixelAccessOptimizer quickly_src(src);

	// Ensure that it is the resized dimentions that are being looped over
	for (int y(0); y < new_height; ++y)
	{
		for (int x(0); x < new_width; ++x)
		{
			// Extrapolate the index of the original image that is to be used this iteration
			// from the resize ratio and current loop counter
			int x_index = (resize_ratio_x * x);
			int y_index = (resize_ratio_y * y);

			// Get the extrapolated pixel from the original image
			COLORREF p1(quickly_src.GetPixel(x_index, y_index));

			BYTE r((p1 >> 16) & 0xff);
			BYTE b(p1 & 0xff);
			BYTE g((p1 >> 8) & 0xff);
		
			// Update the pixel at the new image with the correct pixel from the original image
			quickly_dest.SetPixel(x, y, RGB(r, b, g));
		}
	}
}

void ProcessImage(CImage *img, TCHAR* destFile)
{
	// Create a resized version of the original image for filtering later
	CImage *resizedImage = Resize(img);

	// Use bilinear filtering on the image, using the pixels from the 
	// original image, creating a high quality version of the original image
	// at half size dimentions
	BilinearFilter(img, resizedImage);

	// Brighten the image increasing each colour weight by 10
	// (if they are not already 245 or above)
	Brighten(resizedImage);

	// Modify the colour information to simulate the look of
	// a black and white picture
	Greyscale(resizedImage);

	// Rotate the picture 90 degrees clockwise
	Rotate(resizedImage);

	// Save the processed image under the approprite file name
	resizedImage->Save(destFile);

	// Ensure that any dynamically created CImage objects
	// are deleted correctly
	delete resizedImage;
	delete img;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize Microsoft Foundation Classes, and print an error if failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// Application starts here...

		// Time the application's execution time.
		TIMER start;	// DO NOT CHANGE THIS LINE

		//--------------------------------------------------------------------------------------
		// Process the images...   // Put your code here...

		// List of pre-processed source files
		TCHAR* src_names[12] { L"IMG_1.JPG", 
							   L"IMG_2.JPG", 
							   L"IMG_3.JPG", 
							   L"IMG_4.JPG", 
							   L"IMG_5.JPG", 
							   L"IMG_6.JPG", 
							   L"IMG_7.JPG", 
							   L"IMG_8.JPG", 
							   L"IMG_9.JPG", 
							   L"IMG_10.JPG", 
							   L"IMG_11.JPG", 
							   L"IMG_12.JPG" };

		// List of post-processed desintation files
		TCHAR* dest_names[12]{ L"IMG_1.PNG",
							   L"IMG_2.PNG",
							   L"IMG_3.PNG",
							   L"IMG_4.PNG",
							   L"IMG_5.PNG",
							   L"IMG_6.PNG",
							   L"IMG_7.PNG",
							   L"IMG_8.PNG",
							   L"IMG_9.PNG",
							   L"IMG_10.PNG",
							   L"IMG_11.PNG",
						       L"IMG_12.PNG" };

		// Store the images for processing
		// Images are then later deleted dynamically in the
		// ProcessImage function
		CImage *images[12];

		// One thread for processing each image
		std::thread threads[12];

		// The loading of the images is done on a single Thread
		// When each image is loaded, a Thread is fired off to process that image
		// The main Thread then continues to load the next image
		for (int i(0); i < 12; ++i)
		{
			// CImage requires a dynamically allocated object to work correctly
			images[i] = new CImage;

			// Load the respective image
			images[i]->Load(src_names[i]);

			// Fire off a Thread to process the image just loaded, allowing
			// this Thread to continue loading images
			threads[i] = std::thread(ProcessImage, images[i], dest_names[i]);
		}

		// Ensure that all Threads have rejoined main program
		// before ending
		for (int i(0); i < 12; ++i)
		{
			threads[i].join();
		}

		//-------------------------------------------------------------------------------------------------------

		// How long did it take?...   DO NOT CHANGE FROM HERE...
		
		TIMER end;

		TIMER elapsed;
		
		elapsed = end - start;

		__int64 ticks_per_second = start.get_frequency();

		// Display the resulting time...

		double elapsed_seconds = (double)elapsed.get_time() / (double)ticks_per_second;

		std::cout << "Elapsed time (seconds): " << elapsed_seconds;
		std::cout << std::endl;
		std::cout << "Press a key to continue" << std::endl;

		char c;
		std::cin >> c;
	}

	return nRetCode;
}
