﻿using Microsoft.WindowsAzure.Storage.Table;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace MeasureBridge.StorageService.Interfaces
{
    public interface ITableService<T> where T : ITableEntity, new()
    {
        Task<IEnumerable<T>> GetEntities(int takeCount);

        Task<double> GetAverage(DateTime from, DateTime to);
    }
}
