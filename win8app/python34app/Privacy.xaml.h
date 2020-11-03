//
// Privacy.xaml.h
// Deklaration der Privacy-Klasse
//

#pragma once

#include "Privacy.g.h"

namespace python34app
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class Privacy sealed
	{
	public:
		Privacy();
            void do_close(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
