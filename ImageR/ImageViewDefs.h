#ifndef __IMAGEVIEWDEFS_H__
#define __IMAGEVIEWDEFS_H__

#ifdef _DATA_VIEW_API
#define DATA_VIEW_API __declspec(dllexport)
#else
#define DATA_VIEW_API __declspec(dllimport)
#endif

#endif