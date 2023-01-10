﻿using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using HCM3.Views;
using System.Collections.ObjectModel;
using HCM3.ViewModels;
using HCM3.Models;
using Microsoft.Extensions.DependencyInjection;
using HCM3.Startup;
using System.Diagnostics;
using HCM3.Services;
using HCM3.Services.Trainer;
using HCM3.ViewModels.Commands;
using NonInvasiveKeyboardHookLibrary;


namespace HCM3
{

    
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private readonly ServiceProvider _serviceProvider;

        public App()
        {
            ServiceCollection services = new();
            ConfigureServices(services);

            _serviceProvider = services.BuildServiceProvider();


        }

        private TextWriterTraceListener Logger { get; set; }

        public string CurrentHCMVersion = "2.0.4";

        private void ConfigureServices(ServiceCollection services)
        {


            services.AddSingleton<MainWindow>();

            //ViewModels
            services.AddSingleton<CheckpointViewModel>();
            services.AddSingleton<TrainerViewModel>();
            services.AddSingleton<SettingsViewModel>();
            services.AddSingleton<MainViewModel>();


            //General Services
            services.AddSingleton<CommonServices>();
            services.AddSingleton<DataPointersService>();
            services.AddSingleton<HaloMemoryService>();


            //ViewModel interaction Services
            services.AddSingleton<PersistentCheatService>();
            services.AddSingleton<CheckpointServices>();
            services.AddSingleton<TrainerServices>();
            services.AddSingleton<InternalServices>();

            //Persistent Cheats (note to self, I might want to instead use member injection)
            services.AddSingleton<PC_Invulnerability>();
            services.AddSingleton<PC_Speedhack>();
            services.AddSingleton<PC_BlockCPs>();
            services.AddSingleton<PC_Medusa>();
            services.AddSingleton<PC_OneHitKill>();
            services.AddSingleton<PC_DisplayInfo>();

            services.AddSingleton<HotkeyManager>();
        }

        //might have to remove sender parameter here
        private void OnStartup(object sender, StartupEventArgs e)
        {
            this.Logger = new("file.log");
            Trace.Listeners.Add(this.Logger);

            Trace.WriteLine("OnStartup is run");
            Trace.WriteLine("Current time: " + DateTime.Now);

            HCMSetup setup = new();
            // Run some checks; have admin priviledges, have file access, have required folders & files.
            if (!setup.HCMSetupChecks(out string errorMessage))
            {
                // If a check fails, tell the user why, then shutdown the application.
                System.Windows.MessageBox.Show(errorMessage, "HaloCheckpointManager Error", System.Windows.MessageBoxButton.OK);
                System.Windows.Application.Current.Shutdown();
            }

            var dataPointersService = _serviceProvider.GetService<DataPointersService>();
            // Create collection of all our ReadWrite.Pointers (and other data) and load them from the online repository
            string pointerErrors = "";
            try
            {
                dataPointersService.LoadPointerDataFromSource2(out pointerErrors);
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show(ex.ToString(), "HaloCheckpointManager Error", System.Windows.MessageBoxButton.OK);
                System.Windows.Application.Current.Shutdown();
            }
            if (pointerErrors != "")
            { System.Windows.MessageBox.Show("Some pointers failed to load. Yell at Burnt for making typos." + pointerErrors, "HaloCheckpointManager Error", System.Windows.MessageBoxButton.OK); }

            if (dataPointersService.ObsoleteHCMVersions.Contains(this.CurrentHCMVersion))
            {
                //Tell the user this version of HCM is deprecated and the new version must be downloaded
                System.Windows.MessageBox.Show("Bad HCM version, shutting down", "HaloCheckpointManager Error", System.Windows.MessageBoxButton.OK);
                System.Windows.Application.Current.Shutdown();
            }
            else if (this.CurrentHCMVersion != dataPointersService.LatestHCMVersion)
            {
                //Tell the user a new HCM version exists and ask them if they would like to download it (send them to github release page)
                if (!(MessageBox.Show("A newer version of HCM exists, probably with bugfixes or new features.\nWould you like to go to the HCM releases page now?", "Download HCM update?", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.No))
                {
                    //User clicked yes, so take them to the release page
                    System.Diagnostics.Process.Start(new ProcessStartInfo
                    {
                        FileName = "https://github.com/Burnt-o/HaloCheckpointManager/releases",
                        UseShellExecute = true
                    });
                }
                
            }

            // Tell HaloMemory to try to attach to MCC, both steam and winstore versions
            var haloMemoryService = _serviceProvider.GetService<HaloMemoryService>();
            haloMemoryService.HaloState.ProcessesToAttach = new string[] { "MCC-Win64-Shipping", "MCCWinStore-Win64-Shipping" };
            haloMemoryService.HaloState.TryToAttachTimer.Enabled = true;




            string appTheme = HCM3.Properties.Settings.Default.Darkmode ? "Dark" : "Light";
            this.Resources.MergedDictionaries[0].Source = new Uri($"/Themes/{appTheme}.xaml", UriKind.Relative);

            //this.Resources.MergedDictionaries[0].Source = new Uri($"/Themes/Dark.xaml", UriKind.Relative);







            var mainWindow = _serviceProvider.GetService<MainWindow>();
            mainWindow.DataContext = _serviceProvider.GetService<MainViewModel>();
            mainWindow.Title = "HaloCheckpointManager " + this.CurrentHCMVersion;
            mainWindow.Show();
        }



    }
}
