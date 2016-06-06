//----- Include files -------------------------------------------------------
#include <stdio.h>                 // Needed for printf()

//----- Defines -------------------------------------------------------------
#define MAX               100000        // Maximum number of ticks
#define OUTPUT_STATS       1           // Output statistics flag
#define OUTPUT_SUMMARY     0            // Output summary flag
#define OUTPUT_PARAMETERS  0            // Output parameters flag
#define OUTPUT_LOAD        1            // Output load flag
#define CONTROL_MECH       1            // Turn on the control mechanism

//----- Function prototypes -------------------------------------------------
double ncLoadDevice(double buyPrice, double sellPrice); // Non Curtailable Load device model
double cLoadDevice(double buyPrice, double sellPrice, double cMaxDemand);  // Curtailabe Load device model

//===========================================================================
//=  Main program                                                           =
//===========================================================================
int main(int argc, char *argv[])
{
  double availSource[MAX];    // Available power from source
  double maxBattery;          // Maximum capacity of the battery [J]
  double availBattery;        // Available energy from battery
  double lowBattery;          // Threshold for low battery indication
  double sellPrice;           // Sell price from NG controller
  double cBuyPrice;           // Buy amount set in curtailable load
  double ncBuyPrice;          // Buy amount set in the non curtailable load
  double CRPD1;               // Reduced Power demand from curtailable load 1
  double CRPD2;               // Reduced Power demand from curtailable load 2
  double NCFPD1;              // Power demand from a non curtailable load - 1
  double NCFPD2;              // Power demand from a non curtailable load - 2
  double NCFPD;               // Non curtailable load power demand
  double excess;              // The power wasted per second
  double NCSPD1;              // The power given to the NC Device 1
  double NCSPD2;              // The power given to the NC Device 2
  double CSPD1;               // The power given to the C Device 1
  double CSPD2;               // The power given to the C Device 2
  double sumSellPrice;        // Summary of the sell price 
  double meanSellPrice;       // Mean sell price
  double totalCDemand;        // Total demand from curtailable loads
  double totalNCDemand;       // Total demand from non curtailable loads
  double totalCUnmet;         // Total unmet demand
  double totalNCUnmet;        // Total unmet demand
  double totalExcess;         // Total excess demand
  double totalSource;         // Total Source 
  double remainingEnergy;     // Remaning energy after load consumes
  int    numTicks;            // Number of ticks of source energy
  int    i;                   // Tick number
  int    cutOff;              // Flag to cut off the curtailable load
  double totalEnergy;	        // The total energy available in the system-battery + source
  double wasteTrack;	        // The amount of energy(if any) that is being wasted
  double CFPD1;               // The Maximum Power Demand Curtailable load1
  double CFPD2;               // The Maximum Power Demand Curtailable load2
  double CPD1;                // The Total Power Demand Curtailable load1
  double CPD2;                // The Total Power Demand Curtailable load2
  double curtailThresh1;      // The curtail threshold for the first 12 hours
  double curtailThresh2;      // The curtail threshold for the next 5 hours
  double curtailThresh3;      // The curtail threshold for the final 7 hours
  double sellIncrement;       // The sell price increment for the control mechanism
  int    timer;               // Timer to keep track of the time intervals to send the message
  int    numOfDevices;        // Keeps track of the number of devices in the nanogrid
  double sellPriceOld;        // Keeps track of the sell price in the prev time interval to check for a change in the sell price
  int    messageCount;        // The number of messages sent by the controller to the loads every 15 mins
  int    totalMessageCount;   // The total number of messages
  int    ncLoadCounter;       // Counter for the non curtailable load
  int    ncDemandFlag;        // Keeps track of the power consumption mode of the device
  
  FILE *fp;

  fp = fopen("input.txt", "r");
  
  if(fp != NULL)
  {
    while(!feof(fp)) 
    {
        fscanf(fp, "%lf", &availSource[numTicks]);
        numTicks++;
    }
  }
    
  if ( argc < 2 ) /* argc should be 2 for correct execution */
  {
       /* We print argv[0] assuming it is the program name */
       printf( "usage: %s filename", argv[0] );
  }
  
  // Initialize battery level, low priority load thresholds and the local price increment
  sscanf(argv[1],"%lf",&maxBattery);
  sscanf(argv[2],"%lf",&lowBattery);
  availBattery = maxBattery * lowBattery;
  sscanf(argv[3],"%lf",&curtailThresh1);
  sscanf(argv[4],"%lf",&curtailThresh2);
  sscanf(argv[5],"%lf",&curtailThresh3);
  sscanf(argv[6],"%lf",&sellIncrement);

  // Initialize sell price and buy amount
  sellPrice = sellPriceOld = 0.10;
  cBuyPrice = 0.10;
  ncBuyPrice = 0.10;
  
  // Intialize key variables to zero
  excess = CSPD1 = CSPD2 = 0.0;
  cutOff = 0;
  sumSellPrice = totalCDemand = totalNCDemand = totalCUnmet = totalNCUnmet = totalExcess = totalSource = 0.0;
  numOfDevices = 6;
  messageCount = timer = totalMessageCount = 0;
  NCFPD1 = 55.0;
  NCFPD2 = 0.0;
  CFPD1 = CFPD2 = 0;
  CPD1 = 32.0;
  CPD2 = 30.0;
  ncDemandFlag = 1;
  ncLoadCounter = 0;
  NCFPD = NCFPD1;

  // Output simulation parameters
  if(OUTPUT_PARAMETERS == 1)
  {
    printf("------------------------------------------------------------------------------------ \n");
    printf("-- Maximum Battery Capacity      			       = %lf\n", maxBattery);
    printf("-- Threshold of the battery  			           = %lf\n", lowBattery * 100);
    printf("-- Selling Price                             = %f\n", sellPrice);
    printf("-- Curtailable load buyPrice                 = %f\n", cBuyPrice);
    printf("-- Non Curtailable load buyPrice             = %f\n", ncBuyPrice);
    printf("-- Non Curtailable load demand               = %f\n", NCFPD1);
    printf("-- Curtailable load demand 1                 = %lf\n", CPD1);
    printf("-- Curtailable load demand 2                 = %lf\n", CPD2);
  }
  
  if (OUTPUT_STATS == 1)
  {
    //printf("------------------------------------------------------------------------------------ \n");
    printf("Load,Second,source,battery,price,fulldemand,reduceddemand,excess,unmet\n"); 
  }
  
  // Main simulation loop
  for (i=0; i<numTicks; i++)
  { 
    timer++;
    ncLoadCounter++;

    //Determine Max Demand for the curtailable load based on time of day - Has to be done by the load
    if (i >=0 && i < 28801)
    {
      CFPD1 = curtailThresh1*CPD1;
      CFPD2 = curtailThresh1*CPD2;
    }
    else if (i >= 28801 && i <  64801)
    {
      CFPD1 = curtailThresh2*CPD1;
      CFPD2 = curtailThresh2*CPD2;
    }
    else if (i >=64801)
    {
      CFPD1 = curtailThresh3*CPD1;
      CFPD2 = curtailThresh3*CPD2;
    }
    
    //Set the minimum local price value to 1 cent
    if(sellPrice < 0.01)
    {
      sellPrice = 0.01;
      cBuyPrice = sellPrice;
    }
    else
    {
      cBuyPrice = 0.25;
    }
    
    if(sellPrice > 1.0)
      sellPrice = 1.0;
    
    sumSellPrice = sumSellPrice + sellPrice;
    totalCUnmet = totalCUnmet + ((CRPD1 - CSPD1) + (CRPD2 - CSPD2));
    totalNCUnmet = totalNCUnmet + (NCFPD - NCSPD1);
    totalExcess = totalExcess + excess;
    totalCDemand = totalCDemand + (CRPD1 + CRPD2);
    totalNCDemand = totalNCDemand + (NCFPD);
    totalSource = totalSource + availSource[i];
        
   // ***** Nanogrid Controller **********************************************
    // Consume energy and charge/discharge battery
   
    //If there is a change in the local price value, controller sends a message   
    if(sellPriceOld != sellPrice)
    {
      messageCount++;
    }    
    
    //Count the number of messages per hour
    if((timer%3600) == 0)
    {
      totalMessageCount = totalMessageCount + messageCount;
      messageCount = 0;
    }
    
    sellPriceOld = sellPrice;
        
    ncBuyPrice = ncLoadDevice(ncBuyPrice,sellPrice);
    
    if(ncLoadCounter == 1200 && ncDemandFlag == 1)
    {
      NCFPD = NCFPD2;
      ncLoadCounter = 0;
      ncDemandFlag = 0;
      
      remainingEnergy = availSource[i] - NCFPD;
    
      if(NCFPD > availBattery)
        NCSPD1 = availBattery;
      else
        NCSPD1 = NCFPD;
    }
    else if(ncLoadCounter == 1200 && ncDemandFlag == 0)
    {
      NCFPD = NCFPD1;
      ncLoadCounter = 0;
      ncDemandFlag = 1;
      
      remainingEnergy = availSource[i] - NCFPD;
    
      if(NCFPD > availBattery)
        NCSPD1 = availBattery;
      else
        NCSPD1 = NCFPD;
    }
    else
    {
      remainingEnergy = availSource[i] - NCFPD;
    
      if(NCFPD > availBattery)
        NCSPD1 = availBattery;
      else
        NCSPD1 = NCFPD;
    }
    
  
    // Output key statistics for start of this tick
    if(OUTPUT_LOAD == 1)
    {
       printf("Non Curtailable load1,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",
          i, availSource[i], availBattery/maxBattery, sellPrice, NCFPD, NCSPD1, excess, NCFPD-NCSPD1);
    }
    CRPD1 = cLoadDevice(cBuyPrice,sellPrice, CFPD1);
    
    remainingEnergy = remainingEnergy - CRPD1;
   
    if(availBattery < CRPD1)
      CSPD1 = availBattery;
    else
      CSPD1 = CRPD1;
    
    if(OUTPUT_LOAD == 1)
    {    
      printf("Curtailable load 1,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",
        i, availSource[i], availBattery/maxBattery, sellPrice, CRPD1, CSPD1, excess,CRPD1 - CSPD1);
    }
    
    CRPD2 = cLoadDevice(cBuyPrice,sellPrice, CFPD2);
    
    remainingEnergy = remainingEnergy - CRPD2;
    
    if(availBattery < CRPD2)
      CSPD2 = availBattery;
    else
      CSPD2 = CRPD2;
    
    if(OUTPUT_LOAD == 1)
    {
      printf("Curtailable load 2,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",
        i, availSource[i], availBattery/maxBattery, sellPrice, CRPD2, CSPD2, excess,CRPD2 - CSPD2);
    }
    
     if(OUTPUT_LOAD == 1)
    {
      printf("Total Curtailable load,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",
        i, availSource[i], availBattery/maxBattery, sellPrice, CRPD1+CRPD2, CSPD1+CSPD2, excess,
        (CRPD1 + CRPD2)-(CSPD1 + CSPD2));
    }
    
    //If cutOff flag has been set, determine how much of the demand has been refused
    /*if(cutOff == 1)
    {
      cUnmet = cDemand;
      cDemand = 0;
    }
    else
    {
      cUnmet = 0;
    }*/
    
   availBattery = availBattery + remainingEnergy;
   
   if (availBattery < 0)
     availBattery = 0;
   
   /*
   ncBuyPrice = ncLoadDevice(ncBuyPrice,sellPrice,ncDemand);
    remainingEnergy = availSource[i] - ncDemand;
    
    if(ncDemand > availBattery)
      ncUnmet = ncDemand - availBattery;
    else
      ncUnmet = 0;
   
   
  */
    // ***** Nanogrid Controller **********************************************
    // Consume energy and charge/discharge battery
    
    if (availBattery > maxBattery)
    {
      excess = availBattery - maxBattery;
      availBattery = maxBattery;
    }
    else if (availBattery < 0.0)
    {
      availBattery = 0.0;
    }
    else
      excess = 0.0;
      
    if(CONTROL_MECH == 1)
    { 
    // Determine sell price for next tick based on a battery level rule
    if(availBattery < (0.55 * maxBattery))
      {
        sellPrice = sellPrice + (4 * sellIncrement);
      }
      else if(availBattery > (0.55 * maxBattery) && availBattery < (0.65 * maxBattery))
      {
        sellPrice = sellPrice + (2 * sellIncrement);
      }
      else if(availBattery > (0.65 * maxBattery) && availBattery < (0.75 * maxBattery))
      {
        sellPrice = sellPrice + (1 * sellIncrement);
      }
      else if(availBattery > (0.75 * maxBattery) && availBattery < (0.85 * maxBattery))
      {
        sellPrice = sellPrice - (1 * sellIncrement);
      }
      else if(availBattery > (0.85 * maxBattery))
      {
        sellPrice = sellPrice - (2 * sellIncrement);
      }

      
    }
    // ************************************************************************
  }
  
  if (OUTPUT_SUMMARY == 1)
  {
    meanSellPrice = sumSellPrice / numTicks;
    printf("---------------------------------------------------------------\n");
    printf("-- Mean sell price                              = %f \n", meanSellPrice);
    printf("-- Total non curtailable demand (energy)        = %f  \n", totalNCDemand);
    printf("-- Total curtailable demand (energy)            = %f  \n", totalCDemand);
    printf("-- Total Curtailable unmet demand (energy)      = %f  \n", totalCUnmet);
    printf("-- Total Non Curtailable unmet demand (energy)  = %f  \n", totalNCUnmet);
    printf("-- Total excess supply (energy)                 = %f  \n", totalExcess);
    printf("-- Percentage of battery remaining              = %f  \n", availBattery/maxBattery);
    printf("-- Flag                                         = %f  \n", availBattery/maxBattery);
    printf("-- Total Source Energy                          = %f  \n", totalSource);
    printf("-- Total Number of Messages sent                = %d  \n", totalMessageCount);
    printf("---------------------------------------------------------------\n");
  }

  
  return(0);
}


//===========================================================================
//=  Function to model a curtailable load device                            =
//=-------------------------------------------------------------------------=
//=  Inputs: buyPrice, sellPrice                                            =
//=  Returns: demand                                                        =
//===========================================================================
double cLoadDevice(double cBuyPrice, double sellPrice, double CFPD)
{
  double demand;    // Energy demand from load
  // Rule for determining demand as function of buyPrice and sellPrice
  demand = (cBuyPrice / sellPrice) * CFPD;

  // Cannot demand more than LOAD_MAX in any case
  if (demand > CFPD)
    demand = CFPD;

  return(demand);
}


//===========================================================================
//=  Function to model a non curtailable load device                            =
//=-------------------------------------------------------------------------=
//=  Inputs: buyPrice, sellPrice, demand                                    =
//=  Returns: The revised buying price                                      =
//===========================================================================

double ncLoadDevice(double ncBuyPrice, double sellPrice)
{
  ncBuyPrice = sellPrice;
  return(ncBuyPrice);
}

