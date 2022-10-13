﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Diagnostics;

using HCM3.Services.Trainer;

namespace HCM3.ViewModel.Commands
{
    public class ForceCheckpointCommand : ICommand
    {
        internal ForceCheckpointCommand(TrainerViewModel trainerViewModel, TrainerServices trainerServices)
            {
                    this.TrainerServices = trainerServices;
            this.TrainerViewModel = trainerViewModel;
        }

        private TrainerServices TrainerServices { get; init; }
        private TrainerViewModel TrainerViewModel { get; init; }


        public bool CanExecute(object? parameter)
        {
            //return true;
            return TrainerViewModel.SelectedGameSameAsActualGame;
        }

        public void Execute(object? parameter)
        {
            try
            {
                TrainerServices.ForceCheckpoint(TrainerViewModel.SelectedGame);

            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show("Failed to Force Checkpoint! \n" + ex.Message, "HaloCheckpointManager Error", System.Windows.MessageBoxButton.OK);
            }

        }

        public void RaiseCanExecuteChanged()
        {
            App.Current.Dispatcher.Invoke((Action)delegate // Need to make sure it's run on the UI thread
            {
                _canExecuteChanged?.Invoke(this, EventArgs.Empty);
            });

        }

        private EventHandler? _canExecuteChanged;

        public event EventHandler? CanExecuteChanged
        {
            add
            {
                _canExecuteChanged += value;
                CommandManager.RequerySuggested += value;
            }
            remove
            {
                _canExecuteChanged -= value;
                CommandManager.RequerySuggested -= value;
            }
        }
    }
}
