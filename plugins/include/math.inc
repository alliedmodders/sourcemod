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
	 * @return			Positive number.
	 */
	public static any Abs(any number)
	{
		return number >= 0 ? number : (number * -1);
	}
	
	/**
	 * Sets a number to a given min value,
	 * if the value is smaller than min.
	 * 
	 * @param value			Value
	 * @param min			Min Value used as lower border
	 * @return			Correct value not lower than min
	 */
	public static any MinBounds(any value, any min)
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
	public static any MinValue(any a, any b)
	{
		return a < b ? a : b;
	}
	
	/**
	 * Sets a number to a given max value, 
	 * if the value is greater than max.
	 * 
	 * @param value			Value
	 * @param max			Max Value used as upper border
	 * @return			Correct value not upper than max
	 */
	public static any MaxBounds(any value, any max) 
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
	public static any MaxValue(any a, any b) 
	{	
		return a > b ? a : b;
	}
	
	/**
	 * Sets a number within a range of min and max values.
	 * 
	 * @param value			Value
	 * @param min			Min value used as lower border
	 * @param max			Max value used as upper border
	 * @return			Correct value not lower than min and not greater than max.
	 */
	public static any Clamp(any value, any min, any max)
	{
		value = Math.MinBounds(value, min);
		value = Math.MaxBounds(value, max);

		return value;	
	}
}
