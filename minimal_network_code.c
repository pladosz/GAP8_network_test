#include "pmsis.h"
#include "bsp/bsp.h"
#include "bsp/camera/himax.h"
#include "bsp/buffer.h"
#include "stdio.h"
#include "cpx.h"


//~~~~~~~~~~new network code
#include "classification.h"
#include "classificationKernels.h"
#include "gaplib/ImgIO.h"
//~~~~~~~~~~~~~~~~~~~~ new network code


#define CHANNELS 3
#define IO RGB888_IO
#define CAT_LEN sizeof(uint32_t)

#define CAPTURE_DONE_BIT (1 << 0)
#define LED_PIN 2

#define IMG_ORIENTATION 0x0101
#define CAM_WIDTH 324
#define CAM_HEIGHT 324
#define CYCLE_STEPS 24
#define CRTP_PORT_GENERIC_SETPOINT 0x07
#define HOVER_ITERATIONS 75

#define SIZE_1 84 
#define SIZE_2 84
#define SIZE_3 3


//Performance measuring variables
// static uint32_t start = 0;
// static uint32_t networkTime = 0;
// static uint32_t listTime = 0;
// static uint32_t imgTime = 0;
//~~~~~~~~~~new network code
static unsigned char *cameraBuffer;
static unsigned char *Input_2;
// static unsigned char *imageDemosaiced;
// static signed char *imageCropped;
static signed char *Output_1;

static float scale_output = 0.0078125;//0.011393229;
static float scale_output_2 = 0.0078125;//0.00390625;
static int8_t zero_point_output = 0;

static struct pi_device cluster_dev;
static struct pi_cluster_conf cluster_conf;
static struct pi_cluster_task *task;
AT_HYPERFLASH_FS_EXT_ADDR_TYPE __PREFIX(_L3_Flash) = 0;




///////////stack variables///////////////////////
static unsigned char state_output_list[3][7];
static unsigned char (*state_input)[3][7];
static unsigned char img_output_list[SIZE_1][SIZE_2][SIZE_3];
static unsigned char (*img_input)[SIZE_1][SIZE_2][SIZE_3]; 


static void RunNetwork()
{
  __PREFIX(CNN)
  (img_input, state_input, Output_1);
}

typedef struct{
  unsigned char qx, qy, qz, qw;
  unsigned char vx, vy, vz;
} __attribute__((packed)) drone_state_t;


static unsigned char *imgBuff;
static unsigned char *resizedImgBuff;

static pi_task_t task1;
static struct pi_device camera;
static pi_buffer_t buffer;
static EventGroupHandle_t evGroup;
static CPXPacket_t txp;
static CPXPacket_t rxp;
static CPXPacket_t cxp;



pi_buffer_t header;
uint32_t headerSize;
pi_buffer_t footer;
uint32_t footerSize;
pi_buffer_t jpeg_data;


void combined_task(void *parameters) {
  vTaskDelay(2000);



  // pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, resizedImgBuff);
  // pi_buffer_set_format(&buffer, 84, 84, 1, PI_BUFFER_FORMAT_GRAY);


  // header.size = 1024;
  // header.data = pmsis_l2_malloc(1024);

  // footer.size = 10;
  // footer.data = pmsis_l2_malloc(10);

  // // This must fit the full encoded JPEG
  // jpeg_data.size = 1024 * 15;
  // jpeg_data.data = pmsis_l2_malloc(1024 * 15);

  //State Initialization
  cpxPrintToConsole(LOG_TO_CRTP, "waiting for safety...\n");
  pi_time_wait_us(20000000);
  cpxPrintToConsole(LOG_TO_CRTP, "waiting done!\n");
  cpxInitRoute(CPX_T_GAP8, CPX_T_STM32, CPX_F_SYSTEM, &txp.route);
  //assign the input pointers
  state_input = &state_output_list;
  img_input= &img_output_list; 
  cpxPrintToConsole(LOG_TO_CRTP,"assigned input pointers\n");
  //network initialization
    /* Configure CNN task */

  Output_1 = (signed short *)pmsis_l2_malloc(3 * sizeof(signed short));
  if (Output_1 == NULL)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate memory for output\n");
    pmsis_exit(-1);
  }
  cpxPrintToConsole(LOG_TO_CRTP, "Allocated memory for output\n");

  pi_cluster_conf_init(&cluster_conf);
  pi_open_from_conf(&cluster_dev, (void *)&cluster_conf);
  pi_cluster_open(&cluster_dev);
  task = pmsis_l2_malloc(sizeof(struct pi_cluster_task));

  cpxPrintToConsole(LOG_TO_CRTP,"Allocated memory for task\n");

  memset(task, 0, sizeof(struct pi_cluster_task));
  task->entry = &RunNetwork;
  task->stack_size = STACK_SIZE;             // defined in makefile
  task->slave_stack_size = SLAVE_STACK_SIZE; // "
  task->arg = NULL;

  /* Construct CNN */
  int ret = __PREFIX(CNN_Construct)();
  cpxPrintToConsole(LOG_TO_CRTP,"Constructed CNN\n");



  // Get the state
  //cpxPrintToConsole(LOG_TO_CRTP, "trying to get data!\n");
  cpxReceivePacketBlocking(CPX_F_APP, &rxp);

  drone_state_t * drone_state = (drone_state_t*) rxp.data;

  pi_time_wait_us(200000);


  int current_iteration = 0;



  pi_time_wait_us(100000);

  int main_iteration = 0;
  int i,j,k;
  // assign values to the arrays
  for(i = 0; i < SIZE_1; i++) {
    for(j =0; j < SIZE_2; j++) {
      for(k = 0; k < SIZE_3; k++) {
        img_output_list[i][j][k] = 0;
      }
    }
  }
  i = 0;
  j = 0;
  k = 0;
  for(i = 0; i < 3; i++) {
    for(j =0; j < 7; j++) {
      state_output_list[i][j] = 180;
    }
  }
  while (1) {

    cpxReceivePacketBlocking(CPX_F_APP, &rxp);
    drone_state_t * drone_state = (drone_state_t*) rxp.data;

  
    pi_cluster_send_task_to_cl(&cluster_dev, task);
    // float value = triangle_wave(main_iteration) - 0.5;
    cpxPrintToConsole(LOG_TO_CRTP, "outputs are %hhd %hhd %hhd!\n", Output_1[0],Output_1[1],Output_1[2]);

    main_iteration++;

  }
}





void start_example(void)
{
  struct pi_uart_conf conf;
  struct pi_device device;
  pi_uart_conf_init(&conf);
  conf.baudrate_bps = 115200;

  pi_open_from_conf(&device, &conf);
  if (pi_uart_open(&device))
  {
    pmsis_exit(-1);
  }

  cpxInit();
  cpxEnableFunction(CPX_F_CRTP);
  cpxEnableFunction(CPX_F_APP);

  evGroup = xEventGroupCreate();

  BaseType_t xTask;

  xTask = xTaskCreate(combined_task, "combined_task", configMINIMAL_STACK_SIZE * 4,
                      NULL, tskIDLE_PRIORITY + 1, NULL);

  if (xTask != pdPASS)
  {
    pmsis_exit(-1);
  }
  
  while (1)
  {
    pi_yield();
  }
}

int main(void)
{
  pi_bsp_init();

  // Increase the FC freq to 250 MHz
  pi_freq_set(PI_FREQ_DOMAIN_FC, 250000000);
  pi_pmu_voltage_set(PI_PMU_DOMAIN_FC, 1200);

  return pmsis_kickoff((void *)start_example);
}
