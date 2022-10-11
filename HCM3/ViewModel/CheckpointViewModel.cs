﻿using HCM3.ViewModel.MVVM;
using HCM3.Model;
using System.Collections.ObjectModel;
using System.Windows.Input;
using HCM3.Model.CheckpointModels;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows.Controls;
using System.Windows;
using System;
using Microsoft.Xaml.Behaviors;
using System.Collections;
using System.Collections.Specialized;
using System.Windows.Data;
using GongSolutions.Wpf.DragDrop;
using System.Windows.Controls;
using System.Windows.Shapes;
using System.Collections.Generic;
using System.Linq;

namespace HCM3.ViewModel
{

    internal partial class CheckpointViewModel : Presenter, IDropTarget
    {
        private readonly CheckpointModel CheckpointModel;
        public ObservableCollection<Checkpoint> CheckpointCollection { get; private set; }
        public ObservableCollection<SaveFolder> SaveFolderHierarchy { get; private set; }

        public SaveFolder? RootSaveFolder { get; private set; }


        public MainViewModel MainViewModel { get; private set; }

        public MainModel MainModel { get; private set; }

        private Checkpoint? _selectedCheckpoint;
        public Checkpoint? SelectedCheckpoint
        { 
            get { return _selectedCheckpoint; }
            set 
            { 
            _selectedCheckpoint = value;
                OnPropertyChanged(nameof(SelectedCheckpoint));
            }
        }

        

        public CheckpointViewModel(CheckpointModel checkpointModel, MainViewModel mainViewModel, MainModel mainModel)
        {
            this.CheckpointModel = checkpointModel;
            this.CheckpointCollection = CheckpointModel.CheckpointCollection;
            this.SaveFolderHierarchy = CheckpointModel.SaveFolderHierarchy;
            this.RootSaveFolder = CheckpointModel.RootSaveFolder;
            this.PropertyChanged += Handle_PropertyChanged;
            this.MainViewModel = mainViewModel;
            this.MainModel = mainModel;

            ListCollectionView view = (ListCollectionView)CollectionViewSource
                    .GetDefaultView(this.CheckpointCollection);



            view.CustomSort = new SortCheckpointsByLastWriteTime();

        }

        private void Handle_PropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(SelectedCheckpoint))
            {
                Trace.WriteLine("selected checkpoint changed");
                CheckpointModel.SelectedCheckpoint = SelectedCheckpoint;
            }

        }

        public class SortCheckpointsByLastWriteTime : IComparer
        {
            public int Compare(object x, object y)
            {
                Checkpoint cx = (Checkpoint)x;
                Checkpoint cy = (Checkpoint)y;

                if (cx.ModifiedOn == null || cy.ModifiedOn == null)
                { return 0; }

                int? diff =  (int?)(cx.ModifiedOn - cy.ModifiedOn)?.TotalSeconds;
                return diff == null ? 0 : (int)diff;
            }
        }



        private ICommand _dump;
        public ICommand Dump
        {
            get { return _dump ?? (_dump = new DumpCommand(CheckpointModel)); }
            set { _dump = value; }
        }
        private ICommand _inject;
        public ICommand Inject
        {
            get { return _inject ?? (_inject = new InjectCommand(CheckpointModel)); }
            set { _inject = value; }
        }

        public void FolderChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            SaveFolder? saveFolder = (SaveFolder?)e.NewValue;
            Trace.WriteLine("Selected Folder Path: " + saveFolder?.SaveFolderPath);

            if (saveFolder != null)
            {
                Properties.Settings.Default.LastSelectedFolder[MainViewModel.SelectedTabIndex] = saveFolder.SaveFolderPath;
            }
            
            CheckpointModel.SelectedSaveFolder = saveFolder;
            CheckpointModel.RefreshCheckpointList();

            
        }

        public class DumpCommand : ICommand
        {
            public DumpCommand(CheckpointModel checkpointModel)
            {
                CheckpointModel = checkpointModel;
                
                // Note to self. Need to restructure how viewmodel vs model has selectedtabindex, selected checkpoint etc etc. 
                // Also this command should probably be getting handed the viewmodel instead of the model

                //CheckpointModel.MainModel.SelectedTabIndex.PropertyChanged += (obj, args) => { RaiseCanExecuteChanged(); };
                HaloStateEvents.HALOSTATECHANGED_EVENT += (obj, args) => { RaiseCanExecuteChanged(); };
            }

            private CheckpointModel CheckpointModel { get; set; }
            public bool CanExecute(object? parameter)
            {
                //return true;
                Trace.WriteLine("dump Can execute checked");
                return (CheckpointModel.MainModel.SelectedTabIndex == CheckpointModel.MainModel.HaloMemory.HaloState.CurrentHaloState);
            }

            public void Execute(object? parameter)
            {
                try
                {
                    CheckpointModel.TryDump();
                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show("Failed to dump! \n" + ex.Message, "HaloCheckpointManager Error", System.Windows.MessageBoxButton.OK);
                }
                
            }

            public void RaiseCanExecuteChanged()
            {
                App.Current.Dispatcher.Invoke((Action)delegate // Need to make sure it's run on the UI thread
                {
                    _canExecuteChanged?.Invoke(this, EventArgs.Empty);
                });
                
            }

            //should rewrite this to be a regular event.. then we need to raise it from uh selectedTabIndex changing and from CurrentHaloState getting changed.
            //alternatively. have those things raise PropertyChanged, and we just subscribe to that.
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

        public class InjectCommand : ICommand
        {
            public InjectCommand(CheckpointModel checkpointModel)
            {
                CheckpointModel = checkpointModel;
            }

            private CheckpointModel CheckpointModel { get; set; }
            public bool CanExecute(object? parameter)
            {
                //return true;
                Trace.WriteLine("inject Can execute checked");
                return (CheckpointModel.SelectedCheckpoint != null) && (CheckpointModel.MainModel.SelectedTabIndex == CheckpointModel.MainModel.HaloMemory.HaloState.CurrentHaloState);
            }

            public void Execute(object? parameter)
            {
                try
                {
                    CheckpointModel.TryInject();
                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show("Failed to Inject! \n" + ex.Message, "HaloCheckpointManager Error", System.Windows.MessageBoxButton.OK);
                }

            }

            public event EventHandler? CanExecuteChanged
            {
                add
                {
                    CommandManager.RequerySuggested += value;
                }
                remove
                {
                    CommandManager.RequerySuggested -= value;
                }
            }

           
        }

    }




}
