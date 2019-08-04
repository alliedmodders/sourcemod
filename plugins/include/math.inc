#if defined _math_included
  #endinput
#endif
#define _math_included

methodmap Math
{
	/**
	 * Converts a negative value into a positive value.
	 *
	 * @param number		A number that can be positive or negative.
	 * @return				Positive number.
	 */
	public static int Abs(int number)
	{
		return number >= 0 ? number : (number * -1);
	}
	
	/**
	 * Sets a number to a given min value,
	 * if the value is smaller than min.
	 * 
	 * @param value			Value
	 * @param min			Min Value used as lower border
	 * @return				Correct value not lower than min
	 */
	public static int MinBounds(int value, int min)
	{
		return value < min ? min : value;
	}
	
	/**
	 * Returns the smaller of the two numbers.
	 * 
	 * @param a			Value 1
	 * @param b			Value 2
	 * @return			Smaller Value
	 */
	public static int MinValue(int a, int b)
	{
		return a < b ? a : b;
	}
	
	/**
	 * Sets a number to a given max value, 
	 * if the value is greater than max.
	 * 
	 * @param value			Value
	 * @param max			Max Value used as upper border
	 * @return				Correct value not upper than max
	 */
	public static int MaxBounds(int value, int max) 
	{	
		return value > max ? max : value;
	}
	
	/**
	 * Returns the larger of the two numbers.
	 * 
	 * @param a			Value 1
	 * @param b			Value 2
	 * @return			Larger Value
	 */
	public static int MaxValue(int a, int b) 
	{	
		return a > b ? a : b;
	}
	
	/**
	 * Sets a number within a range of min and max values.
	 * 
	 * @param value			Value
	 * @param min			Min value used as lower border
	 * @param max			Max value used as upper border
	 * @return				Correct value not lower than min and not greater than max.
	 */
	public static int Clamp(int value, int min, int max)
	{
		value = Math.MinBounds(value, min);
		value = Math.MaxBounds(value, max);

		return value;	
	}
}

methodmap FMath
{
		/**
	 * Converts a negative value into a positive value.
	 *
	 * @param number		A number that can be positive or negative.
	 * @return				Positive number.
	 */
	public static float Abs(float number)
	{
		return number >= 0 ? number : (number * -1);
	}
	
	/**
	 * Sets a number to a given min value,
	 * if the value is smaller than min.
	 * 
	 * @param value			Value
	 * @param min			Min Value used as lower border
	 * @return				Correct value not lower than min
	 */
	public static float MinBounds(float value, float min)
	{
		return value < min ? min : value;
	}
	
	/**
	 * Returns the smaller of the two numbers.
	 * 
	 * @param a			Value 1
	 * @param b			Value 2
	 * @return			Smaller Value
	 */
	public static float MinValue(float a, float b)
	{
		return a < b ? a : b;
	}
	
	/**
	 * Sets a number to a given max value, 
	 * if the value is greater than max.
	 * 
	 * @param value			Value
	 * @param max			Max Value used as upper border
	 * @return				Correct value not upper than max
	 */
	public static float MaxBounds(float value, float max) 
	{	
		return value > max ? max : value;
	}
	
	/**
	 * Returns the larger of the two numbers.
	 * 
	 * @param a			Value 1
	 * @param b			Value 2
	 * @return			Larger Value
	 */
	public static float MaxValue(float a, float b) 
	{	
		return a > b ? a : b;
	}
	
	/**
	 * Sets a number within a range of min and max values.
	 * 
	 * @param value			Value
	 * @param min			Min value used as lower border
	 * @param max			Max value used as upper border
	 * @return				Correct value not lower than min and not greater than max.
	 */
	public static float Clamp(float value, float min, float max)
	{
		value = FMath.MinBounds(value, min);
		value = FMath.MaxBounds(value, max);

		return value;	
	}
}
