<?php

class ProfReport
{
	public $time;
	public $uptime;
	public $items = array();
}

class ProfReportParser
{
	private $report;
	private $curtype;
	public $last_error;

	public function Parse($file)
	{
		$this->report = FALSE;
		$this->curtype = FALSE;

		if (($contents = file_get_contents($file)) === FALSE)
		{
			$this->last_error = 'File not found';
			return FALSE;
		}
		
		$xml = xml_parser_create();
		xml_set_object($xml, $this);
		xml_set_element_handler($xml, 'tag_open', 'tag_close');
		xml_parser_set_option($xml, XML_OPTION_CASE_FOLDING, false);

		if (!xml_parse($xml, $contents))
		{
			$this->last_error = 'Line: ' . xml_get_current_line_number($xml) . ' -- ' . xml_error_string(xml_get_error_code($xml));
			return FALSE;
		}

		return $this->report;
	}

	public function tag_open($parser, $tag, $attrs)
	{
		if ($tag == 'profile')
		{
			$this->report = new ProfReport();
			$this->report->time = $attrs['time'];
			$this->report->uptime = $attrs['uptime'];
		}
		else if ($tag == 'report')
		{
			$this->curtype = $attrs['name'];
		}
		else if ($tag == 'item')
		{
			if ($this->report === FALSE || $this->curtype === FALSE)
			{
				return;
			}
			$attrs['type'] = $this->curtype;
			$this->report->items[] = $attrs;
		}
	}

	public function tag_close($parser, $tag)
	{
		if ($tag == 'report')
		{
			$this->curtype = FALSE;
		}
	}
}

?>
