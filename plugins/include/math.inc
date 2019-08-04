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
	public static any Abs(any number)
	{
		return number >= 0 ? number : (number * -1);
	}
	
	/**
	 * Sets a number to a given min value
	 * if the value is smaller than min.
	 * 
	 * @param value			Value
	 * @param min			Min Value used as lower border
	 * @return				Correct value not lower than min
	 */
	public static any Min(any value, any min)
	{
		return value < min ? min : value;
	}
	
	/**
	 * Sets a number to a given max value, 
	 * if the value is greater than max.
	 * 
	 * @param value			Value
	 * @param max			Max Value used as upper border
	 * @return				Correct value not upper than max
	 */
	public static any Max(any value, any max) 
	{	
		return value > max ? max : value;
	}
	
	/**
	 * Sets a number within a range of min and max values.
	 * 
	 * @param value			Value
	 * @param min			Min value used as lower border
	 * @param max			Max value used as upper border
	 * @return				Correct value not lower than min and not greater than max.
	 */
	public static any Clamp(any value, any min, any max)
	{
		value = Math.Min(value, min);
		value = Math.Max(value, max);

		return value;	
	}
}
