  //----------------------------------------------------------------------
  // Below three blocks for blending the middle level into the top level
  //----------------------------------------------------------------------
  /*
  int smallBlendLevels[] =          {  0,  1,  1,   1,   1 };
  int smallBlendMagnifyLevels[] =   {  1,  64,  4,  16,  32 };
  int bigBlendLevels[] =            {  0,  1,  1,   1,   1,   1 };
  int bigBlendMagnifyLevels[] =     {  1,  128,  4,  16,  64, 128 };
  
  int smallBlendZLevels1[] =        {  0,  1,  1,   1,   1 };
  int smallBlendZMagnifyLevels1[] = {  1,  64,  4,  16,  32 };
  int smallBlendZLevels2[] =        {  0,  1,  1,   1 };
  int smallBlendZMagnifyLevels2[] = {  1,  4, 16,  32 };

  int bigBlendZLevels1[] =          {  0,  1,  1,   1,   1,   1 };
  int bigBlendZMagnifyLevels1[] =   {  1,  256,  4,  16,  64, 128 };
  int bigBlendZLevels2[] =          {  0,  1,  1,   1,   1 };
  int bigBlendZMagnifyLevels2[] =   {  1,  4, 16,  64, 128 };
  */
  //----------------------------------------------------------------------
  // Below three blocks for NOT blending the middle level into the top level
  //----------------------------------------------------------------------
  /*
  int smallLevels[] =          {  0,  2,  1,   1,   2 };
  int smallMagnifyLevels[] =   {  1,  64,  4,  16,  32 };
  int bigLevels[] =            {  0,  3,  1,   1,   2,   2 };
  int bigMagnifyLevels[] =     {  1,  1,  4,  16,  64, 128 };
  
  int smallZLevels1[] =        {  0,  2,  1,   1,   2 };
  int smallZMagnifyLevels1[] = {  1,  64,  4,  16,  32 };
  int smallZLevels2[] =        {  0,  1,  1,   2 };
  int smallZMagnifyLevels2[] = {  1,  4, 16,  32 };

  int bigZLevels1[] =          {  0,  2,  1,   1,   2,   2 };
  int bigZMagnifyLevels1[] =   {  1,  256,  4,  16,  64, 128 };
  int bigZLevels2[] =          {  0,  1,  1,   2,   2 };
  int bigZMagnifyLevels2[] =   {  1,  4, 16,  64, 128 };

  int *levels1, *levels2, *magnifyLevels1, *magnifyLevels2;
  int totalLevels1, totalLevels2;
  */

  //****************************************************************
  // Check if slide has more than one Z-Level 
  //****************************************************************
  /*
  else if (slide->getTotalZLevels() > 0)
  {
    if (mBaseTotalWidth >= 140000 || mBaseTotalHeight >= 140000)
    {
      totalLevels1 = 6;
      totalLevels2 = 5;
      if (mBlendTopLevel)
      {
        levels1 = bigBlendZLevels1;
        levels2 = bigBlendZLevels2;
        magnifyLevels1 = bigBlendZMagnifyLevels1;
        magnifyLevels2 = bigBlendZMagnifyLevels2;
      }
      else
      {
        levels1 = bigZLevels1;
        levels2 = bigZLevels2;
        magnifyLevels1 = bigZMagnifyLevels1;
        magnifyLevels2 = bigZMagnifyLevels2;
      }
    }
    else
    {
      if (mBlendTopLevel)
      {
        totalLevels1 = 5;
        totalLevels2 = 4;
        levels1 = smallBlendZLevels1;
        levels2 = smallBlendZLevels2;
        magnifyLevels1 = smallBlendZMagnifyLevels1;
        magnifyLevels2 = smallBlendZMagnifyLevels2;
      }
      else
      {
        totalLevels1 = 5;
        totalLevels2 = 4;
        levels1 = smallZLevels1;
        levels2 = smallZLevels2;
        magnifyLevels1 = smallZMagnifyLevels1;
        magnifyLevels2 = smallZMagnifyLevels2;
      }
    }
    int totalBottomZLevels = slide->getTotalBottomZLevels();
    int totalTopZLevels = slide->getTotalTopZLevels();
    int firstDirection;
    int bottomLevel;
    if (totalBottomZLevels == 0)
    {
      firstDirection=0; // no bottom levels do the base level first
      bottomLevel=0;
    }
    else
    {
      firstDirection=1; // do the first bottom Z-level first if it exists

      bottomLevel=totalBottomZLevels-1;
    }
    //****************************************************************
    // Output the first bottom Z-level plus the slide thumbnail
    //****************************************************************
    for (int step=0; step < totalLevels1 && error==0; step++)
    {
      tiled = (step == 1 ? false : true);
      error=outputLevel(levels1[step], tiled, firstDirection, bottomLevel, magnifyLevels1[step]);
    }
    //****************************************************************
    // Output the next three bottom levels
    //****************************************************************
    for (int zLevel = bottomLevel-1; zLevel >= 0; zLevel--)
    {
      for (int step=0; step < totalLevels2 && error==0; step++)
      {
        tiled = (step == 1 ? false : true);
        error=outputLevel(levels2[step], tiled, 1, zLevel, magnifyLevels2[step]);
      }
    }
    //****************************************************************
    // Output the base (middle) level
    //****************************************************************
    for (int step=0; step < totalLevels2 && error==0 && firstDirection != 0; step++)
    {
      tiled = (step == 1 ? false : true);
      error=outputLevel(levels2[step], tiled, 0, 0, magnifyLevels2[step]);
    }
    //****************************************************************
    // Output the last four upper levels
    //****************************************************************
    for (int zLevel = 0; zLevel < totalTopZLevels; zLevel++)
    {
      for (int step=0; step < totalLevels2 && error==0; step++)
      {
        tiled = (step == 1 ? false : true);
        error=outputLevel(levels2[step], tiled, 2, zLevel, magnifyLevels2[step]);
      }
    }
  }
 
