//
// MyWaypointControl.xaml.h
// Declaration of the MyWaypointControl class
//

#pragma once

#include "MyWaypointControl.g.h"
#include "AppViewModel.h"

namespace RoutesOverBingMapsApp
{
    using namespace Windows::UI::Xaml;


    /// <summary>
    /// Enumerates the possible options of input to define a waypoint in the route.
    /// </summary>
    public enum class WaypointInputOption { Address, Coordinates, Pointer };

    /// <summary>
    /// Represents an item for a combo box of options for waypoint input.
    /// </summary>
    [Windows::UI::Xaml::Data::Bindable]
    public ref class WayptInputComboOpt sealed
    {
    private:

        Platform::String ^m_label;

        WaypointInputOption m_option;

    public:

        WayptInputComboOpt(WaypointInputOption option, Platform::String ^name)
            : m_option(option), m_label(name) {}

        property Platform::String ^Label
        {
            Platform::String ^get() { return m_label; }
        }

        property WaypointInputOption Code
        {
            WaypointInputOption get() { return m_option; }
        }
    };


    /// <summary>
    /// The class for this user control that represents a waypoint.
    /// </summary>
    /// <seealso cref="Data::INotifyPropertyChanged" />
    [Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
    public ref class MyWaypointControl sealed : public Data::INotifyPropertyChanged
	{
    private:

        static IObservableVector<WayptInputComboOpt ^> ^optionsOfInput;

        Waypoint ^m_waypoint;

        WaypointInputOption m_optionOfInput;

	public:

		MyWaypointControl();

        virtual event Data::PropertyChangedEventHandler ^PropertyChanged;

        /// <summary>
        /// Sets waypoint data as the view model.
        /// </summary>
        /// <value>
        /// The waypoint object.
        /// </value>
        property Waypoint ^ViewModel
        {
            void set(Waypoint ^value)
            {
                m_waypoint = value;
                PropertyChanged(this, ref new Data::PropertyChangedEventArgs(Platform::StringReference(L"Order")));
                PropertyChanged(this, ref new Data::PropertyChangedEventArgs(Platform::StringReference(L"Address")));
                PropertyChanged(this, ref new Data::PropertyChangedEventArgs(Platform::StringReference(L"Latitude")));
                PropertyChanged(this, ref new Data::PropertyChangedEventArgs(Platform::StringReference(L"Longitude")));
            }
        }

        /// <summary>
        /// Gets the combo box options (for waypoint data input).
        /// </summary>
        /// <value>
        /// The list of options.
        /// </value>
        property IObservableVector<WayptInputComboOpt ^> ^InputOptions
        {
            IObservableVector<WayptInputComboOpt ^> ^get() { return optionsOfInput; }
        }

        /// <summary>
        /// Gets or sets the option of input.
        /// </summary>
        /// <value>
        /// The code for option of input.
        /// </value>
        property WaypointInputOption OptionOfInput
        {
            WaypointInputOption get() { return m_optionOfInput; }

            void set(WaypointInputOption value)
            {
                // when same value, do not retrigger binding update to prevent infinite recursion
                if (m_optionOfInput == value)
                    return;

                m_optionOfInput = value;
                PropertyChanged(this, ref new Data::PropertyChangedEventArgs(Platform::StringReference(L"OptionOfInput")));
            }
        }

        /// <summary>
        /// Gets the waypoint order in the route.
        /// </summary>
        /// <value>
        /// The waypoint order (1-based).
        /// </value>
        property int Order
        {
            int get() { return m_waypoint->Order; }
        }

        /// <summary>
        /// Gets or sets the address.
        /// </summary>
        /// <value>
        /// The waypoint address.
        /// </value>
        property Platform::String ^Address
        {
            Platform::String ^get() { return m_waypoint->GetLocation().address; }

            void set(Platform::String ^value)
            {
                m_waypoint->GetLocation().address = value;
                PropertyChanged(this, ref new Data::PropertyChangedEventArgs(Platform::StringReference(L"Address")));
            }
        }

        /// <summary>
        /// Gets or sets the latitude.
        /// </summary>
        /// <value>
        /// The waypoint latitude.
        /// </value>
        property double Latitude
        {
            double get() { return m_waypoint->GetLocation().coordinates.Latitude; }

            void set(double value)
            {
                m_waypoint->GetLocation().coordinates.Latitude = value;
                PropertyChanged(this, ref new Data::PropertyChangedEventArgs(Platform::StringReference(L"Latitude")));
                OnTextChangedInput();
            }
        }

        /// <summary>
        /// Gets or sets the longitude.
        /// </summary>
        /// <value>
        /// The waypoint longitude.
        /// </value>
        property double Longitude
        {
            double get() { return m_waypoint->GetLocation().coordinates.Longitude; }

            void set(double value)
            {
                m_waypoint->GetLocation().coordinates.Longitude = value;
                PropertyChanged(this, ref new Data::PropertyChangedEventArgs(Platform::StringReference(L"Longitude")));
                OnTextChangedInput();
            }
        }

        void OnTextChangedInput();

        void OnDataContextChanged(FrameworkElement ^sender, DataContextChangedEventArgs ^evArgs);

        void OnSelChangedInputOptCBox(Platform::Object ^sender, RoutedEventArgs ^evArgs);

        void OnTextChangedAddrASBox(Controls::AutoSuggestBox ^sender, Controls::AutoSuggestBoxTextChangedEventArgs ^evArgs);

        void OnSuggestionChosenAddrASBox(Controls::AutoSuggestBox ^sender, Controls::AutoSuggestBoxSuggestionChosenEventArgs ^evArgs);

        void OnQuerySubmittedAddrASBox(Controls::AutoSuggestBox ^sender, Controls::AutoSuggestBoxQuerySubmittedEventArgs ^evArgs);

        void OnClickLocateButton(Platform::Object ^sender, RoutedEventArgs ^evArgs);
	};

}// end of namespace RoutesOverBingMapsApp
