using System.Threading.Tasks;
using MeasureBridge.Model.Report;
using MeasureBridge.StorageService.Interfaces;
using Microsoft.AspNetCore.Mvc;

namespace MeasureBridge.WebApi.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class ACController : ControllerBase
    {
        protected IACService _service;
        public ACController(IACService service)
        {
            _service = service;
        }

        [HttpGet]
        public async Task<ActionResult<ReportModel>> Get()
        {
            var data = await _service.GetReport();

            return data;
        }
    }
}