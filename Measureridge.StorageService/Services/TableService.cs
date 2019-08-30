using MeasureBridge.Model.Storage;
using MeasureBridge.StorageService.Interfaces;
using MeasureBridge.StorageService.Extensions;

using Microsoft.Extensions.Configuration;
using Microsoft.WindowsAzure.Storage;
using Microsoft.WindowsAzure.Storage.Table;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace MeasureBridge.StorageService.Services
{
    public class TableService<T> : ITableService<T> where T : ITableEntity, new()
    {
        private readonly CloudTableClient _tableClient;

        public TableService(IConfiguration configuration)
        {
            var cloudStorageAccount = CloudStorageAccount.Parse(configuration.GetConnectionString("DefaultConnection"));
            _tableClient = cloudStorageAccount.CreateCloudTableClient();
        }

        public async Task<IEnumerable<T>> GetEntities(DateTime from, DateTime to)
        {
            CloudTable table = _tableClient.GetTableReference(typeof(T).Name);
            var filter = TableQuery.CombineFilters(TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, from),
                TableOperators.And,
                TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, to));
            TableQuery<T> query = new TableQuery<T>().Where(filter);
            var result = await table.ExecuteQueryAsync(query);

            return result;
        }

        public async Task<IEnumerable<T>> GetEntities(int takeCount)
        {
            CloudTable table = _tableClient.GetTableReference(typeof(T).Name);
            TableQuery<T> query = new TableQuery<T>().Take(takeCount);

            var segmentResult = await table.ExecuteQueryAsync(query);

            return segmentResult;
        }
    }
}
