//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "AppViewModel.h"

namespace RoutesOverBingMapsApp
{
	/// <summary>
	/// The main page with a map control in the background.
	/// </summary>
    public ref class MainPage sealed
	{
    private:

        AppViewModel ^m_viewModel;

        int m_nextWayptOrderNum;

	public:

		MainPage();

        property AppViewModel ^ViewModel
        {
            AppViewModel ^get() { return m_viewModel; }
        }

        void OnCheckUseMicrosoftButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnCheckUseGoogleButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnCheckUseTomtomButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnCheckUseAllServicesButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnSelectWaypoint(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnClickAddWaypointButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnClickRemoveButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnClickClearWaypointsButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnClickClearRoutesButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);

        void OnClickFindRouteButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);
    };

}// end of namespace RoutesOverBingMapsApp
