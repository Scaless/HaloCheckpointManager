﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace HCM3.Views.Controls.Trainer.ButtonViews.ButtonOptions
{
    /// <summary>
    /// Interaction logic for TeleportOptionsView.xaml
    /// </summary>
    public partial class LaunchOptionsView : Window
    {
        public LaunchOptionsView()
        {
            InitializeComponent();
        }

        private void CloseOptionsWindow(object sender, RoutedEventArgs e)
        {
                this.Close();
        }
    }
}
