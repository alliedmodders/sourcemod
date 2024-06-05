<?php

class ProfReport
{
	public $time;
	public $uptime;
	public array $items = [];
}

class ProfReportParser
{
	private bool|ProfReport $report = false;
	private bool|string $curtype = false;
	public string $last_error;

	public function Parse(string $file): bool|ProfReport
	{
		$this->report = false;
		$this->curtype = false;

		if (($contents = file_get_contents($file)) === false) {
			$this->last_error = 'File not found';
			return false;
		}
		
		$xml = xml_parser_create();
		xml_set_object($xml, $this);
		xml_set_element_handler($xml, 'tag_open', 'tag_close');
		xml_parser_set_option($xml, XML_OPTION_CASE_FOLDING, false);

		if (!xml_parse($xml, $contents)) {
			$this->last_error = 'Line: ' . xml_get_current_line_number($xml) . ' -- ' . xml_error_string(xml_get_error_code($xml));
			return false;
		}

		return $this->report;
	}

	public function tag_open($parser, string $tag, array $attrs): void
	{
		if ($tag === 'profile') {
			$this->report = new ProfReport();
			$this->report->time = $attrs['time'];
			$this->report->uptime = $attrs['uptime'];
		} elseif ($tag === 'report') {
			$this->curtype = $attrs['name'];
		} elseif ($tag === 'item') {
			if ($this->report === false || $this->curtype === false) {
				return;
			}

			$attrs['type'] = $this->curtype;
			$this->report->items[] = $attrs;
		}
	}

	public function tag_close($parser, string $tag): void
	{
		if ($tag === 'report') {
			$this->curtype = false;
		}
	}
}
