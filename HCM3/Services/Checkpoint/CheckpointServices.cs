﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HCM3.Services
{
    public partial class CheckpointServices
    {
        public HaloMemoryService HaloMemoryService { get; init; }
        public DataPointersService DataPointersService { get; init; }

        public CommonServices CommonServices { get; init; }
        public CheckpointServices(HaloMemoryService haloMemoryService, DataPointersService dataPointersService, CommonServices commonServices)
        { 
        this.HaloMemoryService = haloMemoryService;
        this.DataPointersService = dataPointersService;
            this.CommonServices = commonServices;   
        }
        
    }
}