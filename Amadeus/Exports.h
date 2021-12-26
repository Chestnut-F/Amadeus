#pragma once

#ifndef _AmadeusExport
#	ifdef AMADEUS_EXPORTS
		/* We are building this library */
#		define _AmadeusExport __declspec(dllexport)
#    else
		/* We are using this library */
#      define _AmadeusExport __declspec(dllimport)
#    endif
#endif